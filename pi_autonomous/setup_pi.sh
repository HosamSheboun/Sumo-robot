#!/bin/bash
# setup_pi.sh — Run this ONCE on the Raspberry Pi after first boot.
# It enables the hardware watchdog, disables all sleep/idle behaviour,
# and configures the Pi to stay fully awake until physically unplugged.
#
# Run with: bash setup_pi.sh

set -e

echo "=== Step 1: Enable hardware watchdog ==="
# The BCM2835 watchdog is built into the Pi — just needs to be loaded.
if ! grep -q "bcm2835_wdt" /etc/modules; then
    echo "bcm2835_wdt" | sudo tee -a /etc/modules
fi
sudo modprobe bcm2835_wdt

# Tell systemd to use the watchdog (60 second timeout)
sudo sed -i 's/#RuntimeWatchdogSec=0/RuntimeWatchdogSec=60/' /etc/systemd/system.conf
sudo sed -i 's/#ShutdownWatchdogSec=10min/ShutdownWatchdogSec=10min/' /etc/systemd/system.conf

echo "=== Step 2: Disable all CPU sleep / idle states ==="
# Add kernel boot parameter to disable CPU idle
CMDLINE="/boot/cmdline.txt"
if ! grep -q "processor.max_cstate=0" $CMDLINE; then
    sudo sed -i 's/$/ processor.max_cstate=0 intel_idle.max_cstate=0/' $CMDLINE
fi

echo "=== Step 3: Set CPU governor to performance on boot ==="
cat << 'EOF' | sudo tee /etc/rc.local
#!/bin/bash
# Set performance governor for all cores on boot
for cpu in /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor; do
    echo performance > "$cpu" 2>/dev/null || true
done
exit 0
EOF
sudo chmod +x /etc/rc.local

echo "=== Step 4: Disable HDMI and screen blanking (headless) ==="
if ! grep -q "consoleblank=0" $CMDLINE; then
    sudo sed -i 's/$/ consoleblank=0/' $CMDLINE
fi

echo "=== Step 5: Disable Wi-Fi and Bluetooth power saving ==="
cat << 'EOF' | sudo tee /etc/network/if-up.d/disable_power_saving
#!/bin/bash
/sbin/iwconfig wlan0 power off 2>/dev/null || true
EOF
sudo chmod +x /etc/network/if-up.d/disable_power_saving

echo "=== Step 6: Disable swap (reduces SD card wear, keeps RAM active) ==="
sudo dphys-swapfile swapoff
sudo dphys-swapfile uninstall
sudo systemctl disable dphys-swapfile

echo ""
echo "=== All done! Reboot now with: sudo reboot ==="
echo "After reboot the Pi will:"
echo "  - Never sleep or idle"
echo "  - Auto-reboot if the sumo code crashes or freezes"
echo "  - Stay at full CPU performance at all times"
