# Box-Style Sumo Robot — CAD Concept v2

This package contains a compact, box-shaped sumo-robot chassis concept based on the requested parts and a 200 × 200 mm maximum footprint. The enclosure is designed to protect electronics from side impacts, provide a replaceable low front wedge, and keep the main power packs low in the chassis.

> **Status: fit-check prototype, not a verified final manufacturing file.** The meshes are closed/watertight, but motor brackets, caster, battery holders, wheel couplers, breadboard and sensor boards are represented by approximate envelopes. Measure the real parts and adjust the source before the final print.

## Target geometry

- **Overall target envelope:** about **194 mm wide × 184 mm long × 86 mm high**, including the 65 × 26 mm rear wheels and front wedge.
- **Main enclosure:** 140 × 148 mm outside; 5 mm nominal walls and floor, plus a 15 mm-tall lower perimeter guard that ends about 3 mm above the ring to reduce side/rear underbody access.
- **Lid:** removable, 4 mm plate with locating tabs; six lid fastener positions.
- **Electronics tray:** 122 × 120 × 3 mm.
- **Front blade:** about 136 mm wide and projects about 36 mm past the front wall. The nose is approximately 4 mm thick before edge rounding; it is rounded rather than knife-edged and is designed to sit about 0.5 mm above the ring in the nominal CAD geometry.
- **Rear drive:** two external wheels, nominal 65 mm diameter × 26 mm wide, driven by two GA25-370 motor placeholders.
- **Front support:** one metal swivel caster, represented by an approximate 24 × 20 mm mounting pattern at y=22…42 mm; moved slightly rearward to clear the front ring sensor.

The width target leaves only around **6 mm total margin** under the 200 mm limit. Before competition use, measure the *outermost* wheel/coupler/motor arrangement and blade tip. Any extra hub or bracket protrusion may push the assembly over the limit.

## Component placement assumptions

The 3D board/motor/battery models are visualization-only placeholders; they are **not printable** parts in the STL exports.

- Raspberry Pi Zero 2 W: approximately 65 × 30 mm, mounted on the upper tray.
- Mini breadboard: assumed to be about **83 × 55 mm**. A full-size 165 × 55 mm breadboard will not fit in this layout without redesign.
- H-bridge: approximate 43 × 43 mm L298N-style board.
- MT3608 boost module: placed flat on the lower-left floor shelf and retained with cable ties through four slots.
- Pi supply battery: one 18650 cell in an assumed 76 × 22 mm holder, mounted crosswise just ahead of the drive pack.
- Drive pack: a three-cell 18650 holder placeholder (about 66 × 70 mm), positioned low in the rear/centre of the chassis.
- HC-SR04: two front-wall ports near the top-front.
- Ring detection: one downward-facing IR sensor near the front (y≈+58 mm) and one near the rear (y≈−58 mm), with underside windows clear of the perimeter guard.
- Jumper wires: leave a loop/clearance around the Pi header, breadboard and driver. Secure the battery and loose wiring so they cannot move under impact.

**Items to measure before final print:** wheel outside width including the coupler; GA25 motor and bracket hole locations; caster plate and wheel height; battery-holder footprints; the exact breadboard; the real L298N board; and IR sensor emitter/window spacing.

## Files

### Editable source
- `Sumo_Box_Concept_v2.scad` — parametric OpenSCAD source. Change the `part` value or use `-D` from the command line to export a particular printable part.

### Print parts
- `Sumo_Box_Chassis_v2.stl` — enclosure body, motor-shaft holes, underside sensor windows, tray supports and blade reinforcement.
- `Sumo_Box_Lid_v2.stl` — removable lid.
- `Sumo_Electronics_Tray_v2.stl` — internal electronics deck.
- `Sumo_Front_Blade_v2.stl` — separate rounded front wedge.

### Preview files
- `Sumo_Box_Exterior_v2.png` — closed exterior view.
- `Sumo_Box_Interior_Exploded_v2.png` — open/assembled component visualization.
- `Sumo_Box_Top_Layout_v2.png` — top-down layout visualization.
- `Sumo_Box_Side_Profile_v2.png` — side-elevation drawing.
- `Sumo_Component_Placement_v2.png` — labelled lower-deck and upper-tray layout.

## Re-exporting from OpenSCAD

With OpenSCAD installed, run these commands in the folder containing the SCAD file:

```bash
openscad -D 'part="chassis"' -o Sumo_Box_Chassis_v2.stl Sumo_Box_Concept_v2.scad
openscad -D 'part="lid"'     -o Sumo_Box_Lid_v2.stl Sumo_Box_Concept_v2.scad
openscad -D 'part="tray"'    -o Sumo_Electronics_Tray_v2.stl Sumo_Box_Concept_v2.scad
openscad -D 'part="blade"'   -o Sumo_Front_Blade_v2.stl Sumo_Box_Concept_v2.scad
```

The default `part = "assembly"` is for visualization; do not export the complete assembly view as a single print mesh. The `print_set` view is only a non-fused layout preview.

## Print and assembly starting point

- **Chassis:** orient the floor flat on the bed, with the open side upward. Start around 5 perimeters and 30–40% infill; PETG, ASA or nylon are preferable to brittle PLA for an impact-prone robot.
- **Lid:** print flat, with at least 4–5 perimeters. The model has six screw positions for M3 fasteners and heat-set inserts; verify insert diameter/depth against the inserts you have.
- **Electronics tray:** print flat; verify the tray screw locations against the internal support rails.
- **Blade:** use the slicer’s “place on face” function to put its broad underside on the bed. Start with 6–8 perimeters and high infill (roughly 60–100%). Use two M4 fasteners and captured M4 nuts at the blade mount. Deburr/sand the print, but **do not sharpen the nose or add a sharp metal edge**.
- Fit-check every motor/wheel/caster before committing to the final material. If needed, adjust motor slots, caster holes, or component envelopes in the SCAD and re-export.
- Put the robot on a flat sacrificial plate and check that the wedge clears the surface without scraping. The perimeter guard is about 3 mm above the ring and the nominal wedge nose clearance is around 0.5 mm; wheel load, caster height, print tolerances and arena flatness can change both. Raise the nose slightly if it catches the floor.
- Check the IR sensors’ working distance with the actual ring surface. The mounts are intended for downward-facing edge detection; their final height and threshold need testing.

## Power and wiring warning

The Raspberry Pi Zero 2 W is specified for a 5 V supply and can require up to 2 A; Raspberry Pi’s guidance for Zero-family supplies lists 5 V / 2.5 A. See the [official Zero documentation](https://www.raspberrypi.com/documentation/computers/zero.html) and [Raspberry Pi hardware power guidance](https://www.raspberrypi.com/documentation/computers/raspberry-pi.html).

Do **not** assume the small MT3608 board can reliably power the Pi just because a listing says “2 A”. Some MT3608 modules specify a much lower continuous output than their peak/current-switch headline (for example, one module listing gives 0.5 A continuous; see the [module specification](https://www.otronic.nl/en/mt3608-2a-max-dc-dc-step-up-power-module-booster-power-module)). Use a properly rated, stable 5.1 V supply/regulator capable of at least 2 A continuous (2.5 A headroom is preferable), and verify voltage under load. A single 18650 boosted to Pi voltage must also be capable of the required input current and be used with suitable protection.

Keep motor power separate from the Pi’s regulated supply. Share the logic ground between Pi and H-bridge as required for control signals, but do not power the Pi from the L298N motor supply/regulator. Secure cells, use protected/appropriate battery hardware, insulate exposed terminals, and test for motor-noise resets before competition.

## Final pre-competition checks

1. Measure the assembled robot with wheels, couplers, blade and fasteners installed; it must remain within 200 × 200 mm.
2. Verify all protrusions and corners are rounded/deburred and that the printed wedge cannot scratch the ring.
3. Confirm the wedge clears the ring on a flat plate while remaining low enough to engage an opponent.
4. Test IR edge detection at the front and rear with motors running.
5. Test Pi power during motor start/stall conditions and check for brownouts or resets.
6. Recheck every screw, battery restraint, wire loop and sensor mount after impact testing.
