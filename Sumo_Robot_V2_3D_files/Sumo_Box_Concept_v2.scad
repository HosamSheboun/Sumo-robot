/*
  Box-style sumo robot chassis concept v2
  - Overall envelope including wheels and blade: approx. 194 x 184 x 86 mm
  - Main enclosure: 140 x 148 mm, with 3 mm nominal-clearance perimeter skirts
  - Rounded, blunt lifting wedge; no knife edge
  - Separate printable parts: chassis, lid, electronics tray, front blade
  - Assembly electronics and wheels are non-printable clearance placeholders

  STATUS: Fit-check prototype; the component models are approximate placeholders.
  IMPORTANT: Confirm actual motor, wheel hub, caster, battery holder, and breadboard
  measurements before printing the final parts. The component models are approximate.
*/

$fn = 40;
part = "assembly"; // [assembly,layout,chassis,lid,tray,blade,print_set]
show_components = true;
show_wheels = true;
explode_lid = true;

// Main envelope (mm)
body_w = 140;
body_l = 148;
wall = 5;
floor_z = 18;
body_top = 82;
lid_t = 4;
corner_r = 7;

// Wheel geometry placeholders
wheel_d = 65;
wheel_w = 26;
wheel_x = 84;
wheel_y = -36;
wheel_z = wheel_d/2 + 1.5;

// Internal structure
tray_z = 55;
tray_t = 3;
tray_w = 122;
tray_l = 120;

// ---------- Common utilities ----------
module rounded_prism(w,l,h,r,z0=0) {
  translate([0,0,z0])
    linear_extrude(height=h, convexity=8)
      offset(r=r) square([w-2*r,l-2*r], center=true);
}

module hole_z(x,y,r,z0,h) {
  translate([x,y,z0]) cylinder(r=r,h=h);
}

module hole_y(x,y,z,r,h=14) {
  translate([x,y,z]) rotate([90,0,0]) cylinder(r=r,h=h,center=true);
}

module slot_hole(x,y,length,width,z0,h,angle=90) {
  // A rounded slot, with its long direction given by angle in the XY plane.
  hull() {
    translate([x-cos(angle)*(length-width)/2, y-sin(angle)*(length-width)/2, z0]) cylinder(d=width,h=h);
    translate([x+cos(angle)*(length-width)/2, y+sin(angle)*(length-width)/2, z0]) cylinder(d=width,h=h);
  }
}

// ---------- Chassis body ----------
module side_bosses() {
  // Six reinforced lid screw bosses, tucked into the inner corners/sides.
  for (x=[-61,61], y=[-65,65])
    translate([x,y,floor_z+wall]) cylinder(r=4.2,h=body_top-(floor_z+wall));
  for (x=[-65.7,65.7])
    translate([x,0,floor_z+wall]) cylinder(r=3.4,h=body_top-(floor_z+wall));
}

module side_rails() {
  // Supports for the removable electronics deck, one on each long side.
  for (s=[-1,1])
    translate([s*62, -2, tray_z-4/2]) cube([11,116,4],center=true);
}

module motor_pads() {
  for (s=[-1,1])
    translate([s*53, wheel_y, floor_z+wall])
      rounded_prism(27,47,3,2,0);
}

module chassis_raw() {
  difference() {
    union() {
      // 5 mm floor and sturdy box walls
      difference() {
        rounded_prism(body_w,body_l,body_top-floor_z,corner_r,floor_z);
        rounded_prism(body_w-2*wall,body_l-2*wall,body_top-floor_z+2,3,floor_z+wall);
      }
      // Low perimeter guard: side/rear/front skirts finish 3 mm above the ring.
      // This reduces the exposed underbody gap while keeping the inner floor elevated for the caster.
      difference() {
        rounded_prism(body_w,body_l,floor_z-3,corner_r,3);
        rounded_prism(body_w-2*wall,body_l-2*wall,floor_z-3+1,corner_r-wall,2.5);
      }
      side_bosses();
      side_rails();
      motor_pads();
      // Reinforcing ribs behind the blade mounting points
      for (x=[-50,50])
        translate([x,68,27]) hull() {
          cube([10,4,18],center=true);
          translate([0,4,9]) cube([10,4,10],center=true);
        }
    }

    // Lid screw holes through the screw bosses
    for (x=[-61,61], y=[-65,65]) {
      hole_z(x,y,1.7,floor_z+wall-0.2,body_top-floor_z+2);
      hole_z(x,y,2.15,body_top-6,6.2); // M3 heat-set insert pocket
    }
    for (x=[-65.7,65.7]) {
      hole_z(x,0,1.7,floor_z+wall-0.2,body_top-floor_z+2);
      hole_z(x,0,2.15,body_top-6,6.2); // M3 heat-set insert pocket
    }

    // Motor output shaft clearance through both side walls (one coaxial pair)
    hole_z(0,0,0.01,-1,1); // no-op anchor for consistent preview/compiler behavior
    translate([0,wheel_y,wheel_z]) rotate([0,90,0]) cylinder(d=9.2,h=body_w+60,center=true);

    // HC-SR04 openings through front wall: transducers sit behind these ports
    for (x=[-11,11]) hole_y(x,72,67,8.15,16);

    // Front blade fasteners; clearance holes align with the blade M4 nut traps
    for (x=[-50,50]) hole_y(x,73,24.5,2.15,16);

    // Caster plate mounting holes on the underside at the front
    for (x=[-12,12], y=[22,42]) hole_z(x,y,1.7,floor_z-0.5,wall+1);

    // Motor mounting slots, adjustable for the motor brackets
    for (s=[-1,1]) {
      slot_hole(s*53,-47,12,4.2,floor_z-0.5,wall+4,90);
      slot_hole(s*53,-25,12,4.2,floor_z-0.5,wall+4,90);
    }

    // 3-cell holder attachment holes (dimensions are placeholders)
    for (x=[-27,27], y=[-56,-12]) hole_z(x,y,1.8,floor_z-0.5,wall+1);

    // Single-cell Pi power holder mounting holes (cell mounted crosswise)
    for (x=[-30,30], y=[11,23]) hole_z(x,y,1.7,floor_z-0.5,wall+1);

    // Four cable-tie slots for the low-mounted MT3608 module on the front-left floor area
    for (x=[-60,-45], y=[16,44]) slot_hole(x,y,6,3.0,floor_z-0.5,wall+1,0);

    // IR sensor windows through the floor; sensors mount beneath the chassis
    for (y=[-58,58]) {
      translate([0,y,floor_z-0.5]) cube([20,8,wall+1],center=true);
      for (x=[-13,13]) hole_z(x,y,1.3,floor_z-0.5,wall+1);
    }
  }
}

// ---------- Detachable electronics tray ----------
module electronics_tray() {
  difference() {
    rounded_prism(tray_w,tray_l,tray_t,6,tray_z);

    // Four mount holes to attach the tray to the inner side rails
    for (x=[-60,60], y=[-53,53]) hole_z(x,y,1.7,tray_z-0.5,tray_t+1);

    // Cable-tie slots around the board and mini-breadboard areas
    for (x=[8,52], y=[-19,52]) slot_hole(x,y,10,3.2,tray_z-0.5,tray_t+1,0);

    // Mounting pattern for Raspberry Pi Zero / Zero 2 W (approx. 58 x 23 mm)
    for (x=[-38-11.5,-38+11.5], y=[16-29,16+29])
      hole_z(x,y,1.6,tray_z-0.5,tray_t+1);

    // H-bridge mounting pattern, generalized for a small L298N-style module
    for (x=[-23-17,-23+17], y=[-38-17,-38+17])
      hole_z(x,y,1.8,tray_z-0.5,tray_t+1);
  }
}

// ---------- Protective lid ----------
module lid() {
  difference() {
    union() {
      rounded_prism(body_w,body_l,lid_t,corner_r,body_top);
      // Small registration tabs locate the lid without a full fragile skirt.
      for (x=[-61.5,61.5], y=[-48,48])
        translate([x,y,body_top-2]) rounded_prism(4,10,2,1,0);
    }
    for (x=[-61,61], y=[-65,65]) {
      hole_z(x,y,1.8,body_top-0.2,lid_t+0.5);
      translate([x,y,body_top+lid_t-2]) cylinder(r1=1.8,r2=2.65,h=2.2); // flush M3 flat-head screw
    }
    for (x=[-65.7,65.7]) {
      hole_z(x,0,1.8,body_top-0.2,lid_t+0.5);
      translate([x,0,body_top+lid_t-2]) cylinder(r1=1.8,r2=2.65,h=2.2);
    }
  }
}

// ---------- Blunt lifting blade / wedge ----------
module blade_raw() {
  // The wedge is a closed trapezoidal ramp. A rounded 4 mm nose sits about 0.5 mm above the ring floor.
  // Raw dimensions chosen so final dimensions remain inside the 200 mm envelope.
  difference() {
    minkowski() {
      polyhedron(
        points=[
          [-67,74.8,5.0], [-67,109,1.3], [-67,109,5.3], [-67,74.8,35.0],
          [ 67,74.8,5.0], [ 67,109,1.3], [ 67,109,5.3], [ 67,74.8,35.0]
        ],
        faces=[
          [0,1,2,3], [4,7,6,5], [0,4,5,1],
          [3,2,6,7], [1,5,6,2], [0,3,7,4]
        ], convexity=8
      );
      sphere(r=0.8,$fn=16);
    }
    // Two M4 clearance holes plus nut traps, open on the flat back face of the blade.
    // Fit nuts before bolting the blade against the chassis front wall.
    for (x=[-50,50]) {
      hole_y(x,74.8,24.5,2.15,24);
      translate([x,76.4,24.5]) rotate([90,0,0]) cylinder(r=3.9,h=4.8,$fn=6,center=true);
    }
  }
}

// ---------- Non-printable component clearance placeholders ----------
module wheel_visual(s) {
  // Wheel outer diameter 65 x width 26 (from the supplied component list)
  color([0.10,0.11,0.13])
    translate([s*wheel_x,wheel_y,wheel_z]) rotate([0,90,0]) cylinder(d=wheel_d,h=wheel_w,center=true,$fn=64);
  color([0.20,0.40,0.68])
    translate([s*wheel_x,wheel_y,wheel_z]) rotate([0,90,0]) cylinder(d=28,h=wheel_w+0.5,center=true,$fn=48);
  color([0.12,0.12,0.14])
    translate([s*wheel_x,wheel_y,wheel_z]) rotate([0,90,0]) cylinder(d=8,h=wheel_w+1,center=true,$fn=32);
}

module motor_visual(s) {
  // Approx. GA25 motor + gearbox + mounting bracket placeholder
  color([0.56,0.58,0.61])
    translate([s*53,wheel_y,36]) cube([23,42,22],center=true);
  color([0.72,0.73,0.75])
    translate([s*62,wheel_y,34]) cube([6,25,22],center=true);
  color([0.28,0.29,0.31])
    translate([s*70,wheel_y,34]) rotate([0,90,0]) cylinder(d=7,h=14,center=true);
}

module caster_visual() {
  // Generic small metal swivel caster placeholder at the front underside
  color([0.44,0.46,0.48])
    translate([0,32,16.5]) cube([30,30,3],center=true);
  color([0.60,0.62,0.64])
    translate([0,32,10]) cube([16,9,12],center=true);
  color([0.12,0.13,0.15])
    translate([0,32,8]) rotate([0,90,0]) cylinder(d=16,h=10,center=true,$fn=48);
}

module battery3_visual() {
  color([0.15,0.15,0.16]) translate([0,-33,25]) cube([66,70,4],center=true);
  for (x=[-21,0,21]) {
    color([0.62,0.42,0.67])
      translate([x,-33,33]) rotate([90,0,0]) cylinder(d=18.4,h=65,center=true,$fn=40);
    color([0.75,0.75,0.77])
      translate([x,-66,33]) rotate([90,0,0]) cylinder(d=17.7,h=1.2,center=true,$fn=32);
  }
}

module battery1_visual() {
  color([0.16,0.16,0.17]) translate([0,17,24.5]) cube([76,22,3],center=true);
  color([0.60,0.42,0.64]) translate([0,17,33]) rotate([0,90,0]) cylinder(d=18.4,h=65,center=true,$fn=40);
}

module pi_visual() {
  // Pi Zero board oriented with its 65 mm side along Y
  color([0.08,0.40,0.24]) translate([-38,16,58.8]) cube([30,65,1.6],center=true);
  // PCB mounting holes
  for (x=[-38-11.5,-38+11.5], y=[16-29,16+29])
    color([0.04,0.05,0.05]) translate([x,y,60.1]) cylinder(d=3.2,h=1.1,center=true,$fn=20);
  // USB/micro-HDMI port blocks
  color([0.68,0.69,0.70]) translate([-38,44,60.8]) cube([10,8,4],center=true);
  color([0.68,0.69,0.70]) translate([-38,38,60.4]) cube([7,5,2.5],center=true);
  // Header strip
  color([0.08,0.08,0.08]) translate([-27,7,61.4]) cube([2.5,50,5],center=true);
}

module breadboard_visual() {
  color([0.90,0.90,0.86]) translate([32,16,62]) cube([55,83,8],center=true);
  // terminal rows as small dark perforation marks, deliberately simplified
  for (x=[14:3:54], y=[-20:4:52])
    color([0.25,0.27,0.28]) translate([x-2,y,66.05]) cylinder(d=0.8,h=0.4,center=true,$fn=8);
}

module hbridge_visual() {
  color([0.08,0.24,0.37]) translate([-23,-38,59.5]) cube([43,43,3],center=true);
  color([0.12,0.13,0.14]) translate([-23,-38,62.5]) cube([27,21,4],center=true);
  color([0.74,0.72,0.65]) translate([-23,-17,60.5]) cube([28,3,7],center=true);
}

module boost_visual() {
  // Small DC-DC module laid flat on the lower-left floor shelf; tie slots retain it.
  color([0.06,0.30,0.22]) translate([-53,30,24.5]) cube([17,37,3],center=true);
  color([0.72,0.48,0.16]) translate([-54,29,27]) cylinder(d=9,h=3,center=true,$fn=24);
}

module ultrasonic_visual() {
  color([0.08,0.40,0.23]) translate([0,68.5,67]) cube([45,2,20],center=true);
  for (x=[-11,11]) {
    color([0.70,0.72,0.74]) translate([x,73,67]) rotate([90,0,0]) cylinder(d=15,h=12,center=true,$fn=48);
    color([0.47,0.49,0.51]) translate([x,78.7,67]) rotate([90,0,0]) cylinder(d=13.3,h=0.6,center=true,$fn=48);
  }
}

module ir_visual(ypos) {
  color([0.06,0.28,0.17]) translate([0,ypos,12]) cube([28,15,3],center=true);
  color([0.09,0.10,0.11]) translate([0,ypos,9.8]) cylinder(d=5.5,h=3.2,center=true,$fn=24);
  color([0.72,0.18,0.14]) translate([7,ypos,9.8]) cylinder(d=4,h=3.2,center=true,$fn=24);
}

module component_placeholders() {
  battery3_visual();
  battery1_visual();
  for (s=[-1,1]) motor_visual(s);
  pi_visual();
  breadboard_visual();
  hbridge_visual();
  boost_visual();
  ultrasonic_visual();
  ir_visual(58);
  ir_visual(-58);
  caster_visual();
}

module assembled_view() {
  color([0.20,0.23,0.26]) chassis_raw();
  color([0.78,0.38,0.12]) blade_raw();
  if (show_wheels) for (s=[-1,1]) wheel_visual(s);
  if (show_components) component_placeholders();
  if (explode_lid) {
    color([0.27,0.31,0.35,0.20]) translate([0,-8,20]) lid();
  } else {
    color([0.27,0.31,0.35]) lid();
  }
}

module layout_view() {
  // Transparency is for visualization only; printed chassis remains opaque.
  color([0.20,0.23,0.26,0.16]) chassis_raw();
  color([0.78,0.38,0.12]) blade_raw();
  if (show_wheels) for (s=[-1,1]) wheel_visual(s);
  if (show_components) component_placeholders();
}

module print_set_preview() {
  // Non-print layout preview only; export each part separately for printing.
  translate([-95,0,0]) chassis_raw();
  translate([95,0,0]) lid();
  translate([0,110,0]) electronics_tray();
  translate([0,-110,0]) blade_raw();
}

if (part == "assembly") {
  assembled_view();
} else if (part == "layout") {
  layout_view();
} else if (part == "chassis") {
  chassis_raw();
} else if (part == "lid") {
  lid();
} else if (part == "tray") {
  electronics_tray();
} else if (part == "blade") {
  blade_raw();
} else if (part == "print_set") {
  print_set_preview();
}
