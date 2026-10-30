"""
bluetooth.py — Dabble Bluetooth joystick interface using Pi Zero 2 W built-in BT.

How Dabble works:
  The Dabble app (iOS / Android) connects to the Pi over Bluetooth Classic
  using the Serial Port Profile (SPP). It sends small data packets containing
  the joystick X and Y values every time the joystick moves.

  Dabble Gamepad joystick packet format (7 bytes):
    Byte 0 : 0xFF  (start marker)
    Byte 1 : 0x01  (Gamepad module ID)
    Byte 2 : 0x02  (Joystick sub-type)
    Byte 3 : X value  (0–14, centre = 7)
    Byte 4 : Y value  (0–14, centre = 7)
    Byte 5 : Buttons bitmask (unused here)
    Byte 6 : 0x00  (end marker)

  We convert X and Y from 0–14 range to -7 to +7 by subtracting 7.

This module:
  1. Registers the Pi as a Bluetooth SPP device named config.BT_NAME
  2. Waits for the Dabble app to connect
  3. Reads joystick packets in a background thread
  4. Exposes get_xy() which returns the latest (x, y) tuple safely
"""

import socket
import threading
import subprocess
import logging
import config

log = logging.getLogger(__name__)

# Dabble packet constants
_START   = 0xFF
_MOD_GP  = 0x01   # Gamepad module
_SUB_JS  = 0x02   # Joystick sub-type
_PKT_LEN = 7
_CENTRE  = 7      # Dabble joystick centre value (range 0–14)


class DabbleBluetooth:
    """
    Manages the Bluetooth SPP connection to the Dabble app.

    Usage:
        bt = DabbleBluetooth()
        bt.start()                    # begin advertising and waiting
        x, y = bt.get_xy()            # get latest joystick values (-7 to +7)
        bt.is_connected()             # True when phone is connected
        bt.stop()                     # clean shutdown
    """

    def __init__(self):
        self._x          = 0      # latest joystick X  (-7 to +7)
        self._y          = 0      # latest joystick Y  (-7 to +7)
        self._connected  = False
        self._running    = False
        self._lock       = threading.Lock()
        self._thread     = None
        self._client_sock = None

    # ── Public ────────────────────────────────────────────────────────────────

    def start(self):
        """Set up Bluetooth and start the background listener thread."""
        self._running = True
        self._setup_bluetooth()
        self._thread = threading.Thread(target=self._listen_loop, daemon=True)
        self._thread.start()
        log.info("Bluetooth started. Open Dabble app and connect to '%s'.",
                 config.BT_NAME)

    def stop(self):
        """Cleanly close the Bluetooth connection."""
        self._running = False
        if self._client_sock:
            try:
                self._client_sock.close()
            except Exception:
                pass
        self._connected = False
        log.info("Bluetooth stopped.")

    def is_connected(self) -> bool:
        """True when the Dabble app is actively connected."""
        with self._lock:
            return self._connected

    def get_xy(self) -> tuple:
        """
        Return the latest joystick (x, y) values.
        Both are in range -7 to +7. (0, 0) = centred = stop.
        """
        with self._lock:
            return self._x, self._y

    def get_xy_normalised(self) -> tuple:
        """
        Return joystick values normalised to -1.0 to +1.0.
        Applies deadzone from config.JOYSTICK_DEADZONE.
        """
        with self._lock:
            x, y = self._x, self._y

        dz = config.JOYSTICK_DEADZONE
        x = 0 if abs(x) <= dz else x
        y = 0 if abs(y) <= dz else y

        norm_x = x / config.JOYSTICK_MAX
        norm_y = y / config.JOYSTICK_MAX

        # Clamp to [-1.0, +1.0]
        norm_x = max(-1.0, min(1.0, norm_x))
        norm_y = max(-1.0, min(1.0, norm_y))
        return norm_x, norm_y

    # ── Private ───────────────────────────────────────────────────────────────

    def _setup_bluetooth(self):
        """
        Configure the Pi's built-in Bluetooth as a discoverable SPP device.
        Runs shell commands to set the device name and make it discoverable.
        """
        cmds = [
            f"sudo hciconfig hci0 name '{config.BT_NAME}'",
            "sudo hciconfig hci0 piscan",       # discoverable + connectable
            "sudo hciconfig hci0 up",
            # Register SPP service so Dabble can find it
            "sudo sdptool add SP",
        ]
        for cmd in cmds:
            try:
                subprocess.run(cmd, shell=True, check=True,
                               capture_output=True, timeout=5)
            except Exception as e:
                log.warning("BT setup command failed: %s — %s", cmd, e)

    def _listen_loop(self):
        """
        Background thread: waits for Dabble to connect, then reads packets.
        Automatically waits for reconnection if the app disconnects.
        """
        # Create RFCOMM Bluetooth socket (SPP)
        server_sock = socket.socket(socket.AF_BLUETOOTH,
                                    socket.SOCK_STREAM,
                                    socket.BTPROTO_RFCOMM)
        server_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        server_sock.bind(("", 1))   # RFCOMM channel 1
        server_sock.listen(1)
        server_sock.settimeout(2.0)

        log.info("Waiting for Dabble app to connect...")

        while self._running:
            try:
                client_sock, addr = server_sock.accept()
                self._client_sock = client_sock
                client_sock.settimeout(1.0)

                with self._lock:
                    self._connected = True
                log.info("Dabble connected from %s", addr)

                self._read_packets(client_sock)

                with self._lock:
                    self._connected = False
                    self._x = 0
                    self._y = 0
                log.info("Dabble disconnected. Waiting for reconnect...")

            except socket.timeout:
                continue
            except OSError:
                break

        server_sock.close()

    def _read_packets(self, sock):
        """Read and parse Dabble joystick packets from an open socket."""
        buf = bytearray()

        while self._running:
            try:
                chunk = sock.recv(32)
                if not chunk:
                    break
                buf.extend(chunk)

                # Parse all complete packets in the buffer
                while len(buf) >= _PKT_LEN:
                    # Find start marker
                    if buf[0] != _START:
                        buf.pop(0)
                        continue

                    # Check we have a full packet
                    if len(buf) < _PKT_LEN:
                        break

                    pkt = buf[:_PKT_LEN]

                    # Validate it is a Gamepad joystick packet
                    if pkt[1] == _MOD_GP and pkt[2] == _SUB_JS:
                        raw_x = pkt[3] - _CENTRE   # convert 0–14 → -7 to +7
                        raw_y = pkt[4] - _CENTRE
                        with self._lock:
                            self._x = raw_x
                            self._y = raw_y

                    # Remove the consumed packet from the buffer
                    del buf[:_PKT_LEN]

            except socket.timeout:
                continue
            except OSError:
                break
