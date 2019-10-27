// -----------------------------------------------------------------------------
// choose what to render
// -----------------------------------------------------------------------------

/*
`make_*` variables render components for 3D printing.

`show_*` variables render auxiliar components in their right position relative to
their corresponding `*_body` component. They are meant for viewing, do not use
them from printing.

E.g. for rendering "assembled" CPU module, set:
make_cpu_body = 1;
show_cpu_pcb = 1;
show_cpu_top_lid = 1;

To render printable CPU module body, set:
make_cpu_body = 1;

To render printable CPU module top lid, set:
make_cpu_top_lid = 1;
*/

/*
// cpu module
make_cpu_body = 0;
make_cpu_top_lid = 0;
show_cpu_top_lid = 0;
show_cpu_pcb = 0;
show_cpu_lid_lock_test = 0;

// Blue Pill IO module
make_bpio_body = 0;
make_bpio_top_lid = 0;
show_bpio_top_lid = 0;
show_bpio_pcb = 0;

// patchbox
make_patchbox_body = 0;
make_patchbox_top_lid = 0;
show_patchbox_top_lid = 0;
show_patchbox_pcb = 0;

// 4relay holder light
make_4relays_holders = 0;

// common
make_bottom_lid = 0;
show_bottom_lid = 0;
*/

// -----------------------------------------------------------------------------
// constants
// -----------------------------------------------------------------------------

// frame thickness
ft = 2;

// clip thickness
clipz = 15;

// bottom lid thickness
blz = 2;

// top lid thickness
tlz = 2;

// top frame height
tfz = 19.7-tlz;

// bottom frame height
bfz = 12.6-blz;

//// PCB dimensions
pcb_x = 70;
pcb_y = 50;
pcb_z = 1.7;
// hole diameter
hd = 3;
// holes distances
pcb_hls_dx = 65.7;
pcb_hls_dy = 45.5;
// hole center offset to the pcb edge
hox = 2;
hoy = 2;

// extra space around pcb
pcb_spc = 1;

// frame inner dimensions (computed)
ix = pcb_x + 2*pcb_spc;
iy = pcb_y + 2*pcb_spc;
iz = bfz + pcb_z + tfz;

// frame outer dimensions (computed)
ox = ix + 2*ft;
oy = iy + 2*ft;
oz = iz;

//// connector & LEDs frame holes dimensions (without extra space)
// extra space around connectors
fhes = 1;

// JST-XH (CAN)
c_jstxh_x = 12.25;
c_jstxh_y = 7;
c_jstxh_z = 6;

// KF2510 (power)
c_kf2510_x = 11;
c_kf2510_y = 11.7; // receptacle depth
c_kf2510_z = 6.3;

// 2x2 LEDs matrix
c_leds2x2_x = 9;
c_leds2x2_z = 9;

// USB
c_usb_y = 13;
c_usb_z = 7.5;
c_usb_dz = 8.5; // relative to top PCB surface

// IDC 2x5 receptacle
c_idc2x5_x = 20.33;
c_idc2x5_y = 8.8;
c_idc2x5_z = 9;

// keystone dimensions
c_keystn_x = 17.7;
c_keystn_y = 11.5;
c_keystn_z = 25;

//// common constants
support_width = 0.8;

//// standoffs
stndoff_x = 5.5+pcb_spc;

//// connector holes positions (relative to the PCB edge) - common
can_dy = 6;
pwr_dy = can_dy + 2 * c_jstxh_x + 2;

//// connector holes positions (relative to the PCB edge) - cpu
cpu_leds2x2_dx = 50;
cpu_usb_dy = 9;

//// connector holes positions (relative to the PCB edge) - patchbox
// keystone
patchbox_keystone_dx = ox - ft - c_keystn_z - 7;
// pwr
patchbox_pwr_dx = 8;

//// connector holes positions (relative to the PCB edge) - bpio
// USB
bpio_usb_dy = 6;
bpio_usb_dz = 9;
// IDC
bpio_idc_dx = 43.6;

//// lid constants
// lid snap frame dimensions
lid_frame_t = 2;
lid_frame_z = 4;

// lid lock thickness
lid_lock_t = 0.6;

// space between body wall and lid frame
lid_spc = lid_lock_t - 0.1;

// lid frame holes margin (make holes in lid frame this wider than holes themselves)
lid_frame_hole_margin = 1;

// assert available in Openscad >=2019.05 only

// 35mm = 2 DIN rail modules
//assert(oz+tlz+blz <= 35);
echo("should be true: 35 >=", oz+tlz+blz);

// -----------------------------------------------------------------------------
// common
// -----------------------------------------------------------------------------

module din_rail_clip(z = 6, spring_thickness=2) {
    a = 7;
    b = 1.2; // sheet thickness, should be ~ 1
    c = 2.5;
    d = 4;
    e = 3;
    f = 4;
    g = 4;
    h = 13.3; // should be 13
    i = 1.5;
    j = spring_thickness;
    k = 25;
    //l = 5.5;
    l = 0;

    //assert(e+f+g+h+g+f+e == 35);
    echo("this should be 35 ... ", e+f+g+h+g+f+e);

    linear_extrude(height=z)
    polygon(points=[
        [j+e+f+g+h+g+f+e+d, 0], // 0
        [j+e+f+g+h+g+f+e+d, a+b+c],
        [j+e+f+g+h+g+f, a+b+c],
        //[j+e+f+g+h+g+f, a+b],
        [j+e+f+g+h+g+f, a+b+(c/4)],
        [j+e+f+g+h+g+f+e, a+b],
        [j+e+f+g+h+g+f+e, a], // 5
        [j+e+f+g+h+g, a],
        [j+e+f+g+h+g, a+l],
        [j+e+f+g+h, a+l],
        [j+e+f+g+h, a],
        [j+e+f+g, a], // 10
        [j+e+f+g, a+l],
        [j+e+f, a+l],
        [j+e+f, a],
        [j+i, a],
        [j+i, 0], // 15
        [j, 0],
        [j, a+b],
        [j+e, a+b],
            [j+e, a+b+(c/3)],
        [j, a+b+c],
        [-k, a+b+c], // 20
        [-k, a+b],
        [0, a+b],
        [0, 0]
    ]);

    // demo base
    //translate([-3,-3,0]) cube([3+j+e+f+g+h+g+f+e+d+3, 3, z]);

}

module frame() {
    difference() {
        cube([ox, oy, oz]);
        translate([ft,ft,0]) cube([ix, iy, iz]);
    }
}

module standoffs(fourth=true) {
    z = bfz;
    // hole dimensions
    hr = hd / 2;
    hz = bfz;
    fn = 20;
    // holes offsets
    ox = ft + pcb_spc + hox;
    oy = ft + pcb_spc + hoy;
    difference(){
        // boxes
        translate([ft,ft,0]) {
            cube([stndoff_x,stndoff_x,z]);
            translate([ix-stndoff_x,0,0]) cube([stndoff_x,stndoff_x,z]);
            translate([0,iy-stndoff_x,0]) cube([stndoff_x,stndoff_x,z]);
            if(fourth) translate([ix-stndoff_x,iy-stndoff_x,0]) cube([stndoff_x,stndoff_x,z]);
        }
        // holes
        translate([ox,oy,0]) {
            cylinder(r=hr, h=hz, $fn=fn);
            translate([pcb_hls_dx, 0, 0]) cylinder(r=hr, h=hz, $fn=fn);
            translate([0, pcb_hls_dy, 0]) cylinder(r=hr, h=hz, $fn=fn);
            if(fourth) translate([pcb_hls_dx, pcb_hls_dy, 0]) cylinder(r=hr, h=hz, $fn=fn);
        }
    }
}

module led_hole() {
    // 3mm LEDs
    r = 3.1/2;
    fn = 20;
    translate([r,ft,r]) rotate([90,0,0]) cylinder(r=r, h=ft, $fn=fn);
}

module lid(with_ventilation=0) {
    // lock length (proportional) [0-1]
    llp = 0.9;

    // cylinder
    r = lid_lock_t;
    fn = 20;

    ///// computed
    lox = ix - 2*lid_spc;
    loy = iy - 2*lid_spc;
    lix = lox - 2*lid_frame_t;
    liy = loy - 2*lid_frame_t;

    // lid
    difference() {
        cube([ox, oy, tlz]);
        // ventilation
        if (with_ventilation) {
            vr = 2;
            vd = vr*4;
            for (x=[0:vd:ox])
            for (y=[0:vd:oy])
                translate([x,y,0]) cylinder(r=vr, h=tlz, $fn=fn);
        }
    }

    // frame
    translate([ft+lid_spc,ft+lid_spc,tlz]) {
        difference() {
            cube([lox, loy, lid_frame_z]);
            translate([lid_frame_t,lid_frame_t,0]) cube([lix, liy, lid_frame_z]);
        }
        // locks
        translate([0,0,lid_frame_z/2]) {
            translate([lox*((1-llp)/2),0,0]) rotate([0,90,0]) cylinder(r=r, h=lox*llp, $fn=fn);
            translate([lox*((1-llp)/2),loy,0]) rotate([0,90,0]) cylinder(r=r, h=lox*llp, $fn=fn);
            translate([0,loy*((1-llp)/2),0]) rotate([-90,0,0]) cylinder(r=r, h=loy*llp, $fn=fn);
            translate([lox,loy*((1-llp)/2),0]) rotate([-90,0,0]) cylinder(r=r, h=loy*llp, $fn=fn);
        }
    }
}

// -----------------------------------------------------------------------------
// CPU
// -----------------------------------------------------------------------------

module cpu_leds() {
    // centers distance
    cd = 2.54*2;

    led_hole();
    translate([0,0,cd]) led_hole();
    translate([cd,0,0]) led_hole();
    translate([cd,0,cd]) led_hole();
}

module cpu_top_lid() {
    difference() {
        top_lid();
        translate([0, 0, bfz+pcb_z]) {
            // USB connector hole
            translate([ox-ft-lid_spc-lid_frame_t, ft+pcb_spc+cpu_usb_dy-fhes-lid_frame_hole_margin, c_usb_dz-fhes]) cube([ft+lid_spc+lid_frame_t, c_usb_y+2*fhes+lid_frame_hole_margin*2, c_usb_z+2*fhes+lid_frame_hole_margin]);
        }
    }
}

module bottom_lid() {
    // offset to remove locks
    off = 1;
    // standoff margin
    sm = 2;
    // standoffs dimensions
    sox = stndoff_x + 2*sm;
    soz = lid_frame_z;

    difference() {
        translate([0,0,-tlz]) lid();
        // standoffs space
        translate([ft,ft,0]) {
            translate([-off, -off, 0]) cube([sox,sox,soz]);
            translate([ix-sox+off, -off,0]) cube([sox,sox,soz]);
            translate([-off,iy-sox+off,0]) cube([sox,sox,soz]);
            translate([ix-sox+off,iy-sox+off,0]) cube([sox,sox,soz]);
        }
    }
}

module cpu_body(with_supports=1, with_power_connector=0, with_one_led_hole=0) {
    translate([10,0,0]) mirror([0,1,0]) din_rail_clip(z=clipz);
    difference(){
        frame();
        translate([0, 0, bfz+pcb_z]) {
            // CAN connector hole
            translate([0, ft+pcb_spc+can_dy-fhes, 0]) cube([ft, 2*c_jstxh_x+2*fhes, c_jstxh_z+fhes]);
            // pwr connector hole
            if (with_power_connector)
                translate([0, pwr_dy-fhes, 0]) cube([ft, c_kf2510_x+2*fhes, tfz]);
            if (with_one_led_hole) {
                // LEDs - one square hole
                translate([ft+pcb_spc+cpu_leds2x2_dx-fhes, oy-ft, 0])
                    cube([c_leds2x2_x+2*fhes, ft, c_leds2x2_z]);
            } else {
                // individual LEDs
                translate([ft+pcb_spc+cpu_leds2x2_dx, oy-ft, 0]) cpu_leds();
            }
            // USB connector hole
            translate([ox-ft, ft+pcb_spc+cpu_usb_dy-fhes, c_usb_dz-fhes]) cube([ft, c_usb_y+2*fhes, c_usb_z+2*fhes]);
        }
        // subtract lid locks
        cpu_top_lid();
    }
    // supports
    if (with_supports) {
        for(i=[0.33, 0.66]) {
            translate([0, 0, bfz+pcb_z]) {
                // CANs supports
                translate([0, ft+pcb_spc+can_dy-fhes, 0]) {
                    translate([0, (2*c_jstxh_x+2*fhes)*i, 0]) cube([ft, support_width, c_jstxh_z+fhes]);
                }
                // USB supports
                translate([ox-ft, ft+pcb_spc+cpu_usb_dy-fhes, c_usb_dz-fhes]) {
                    translate([0,(c_usb_y+2*fhes)*i,0]) cube([ft, support_width, c_usb_z+2*fhes]);
                }
            }
        }
    }

    standoffs();
}

module top_lid() {
    translate([ox,0,oz+tlz]) rotate([0,180,0]) lid();
}

// -----------------------------------------------------------------------------
// Blue Pill IO module
// -----------------------------------------------------------------------------

module bpio_body(with_supports=1, with_usb=0) {
    difference() {
        union() {
            translate([10,0,0]) mirror([0,1,0]) din_rail_clip(z=clipz);
            difference(){
                frame();
                translate([0, 0, bfz+pcb_z]) {
                    // CAN connector hole
                    translate([0, ft+pcb_spc+can_dy-fhes, 0]) cube([ft, 2*c_jstxh_x+2*fhes, c_jstxh_z+fhes]);
                    // IDC connector hole
                    translate([ft+pcb_spc+bpio_idc_dx-fhes, oy-ft, 0]) cube([c_idc2x5_x+2*fhes, ft, c_idc2x5_z+fhes]);
                    // USB connector hole
                    if(with_usb) {
                        translate([ox-ft, ft+pcb_spc+bpio_usb_dy-fhes, bpio_usb_dz-fhes]) cube([ft, c_usb_y+2*fhes, c_usb_z+2*fhes]);
                    }
                }
            }
            standoffs();
            if (with_supports) {
                for(i=[0.33, 0.66]) {
                    // IDC hole supports
                    translate([ft+pcb_spc+bpio_idc_dx-fhes, oy-ft, bfz+pcb_z]) {
                        translate([(c_idc2x5_x+2*fhes)*i, 0, 0]) cube([support_width, ft, c_idc2x5_z+fhes]);
                    }
                    // CANs supports
                    translate([0, ft+pcb_spc+can_dy-fhes, bfz+pcb_z]) {
                        translate([0, (2*c_jstxh_x+2*fhes)*i, 0]) cube([ft, support_width, c_jstxh_z+fhes]);
                    }
                }
            }
        }
        top_lid();
        bottom_lid();
    }
}

module keystone_receptacle() {
    translate([c_keystn_x, c_keystn_y, 0])
    rotate([90,0,180])
    translate([-3.65,-11.3,-11.5]) {
        difference() {
            translate([10,30,0])  import("Small_Single_Keystone_Jack_Faceplate.stl", convexity=3);
            cube(size=[25, 11.3, 2.2], center=false);
            translate([0,36.3,0]) cube(size=[25, 8.7, 2.2], center=false);
            translate([0,11.3,0]) cube(size=[3.65, 25, 2.2], center=false);
            translate([21.35,11.3,0]) cube(size=[3.65, 25, 2.2], center=false);
        }
        translate([10,35.3,0]) cube(size=[5, 1, 2.2], center=false);
    }
}
module keystone_hole() {
    cube([c_keystn_x, c_keystn_y, c_keystn_z]);
}

module keystone_receptacle_laying() {
    translate([c_keystn_z, c_keystn_y, c_keystn_x]) rotate([0,90,180])
        keystone_receptacle();
}
module keystone_hole_laying() {
    translate([0, 0, c_keystn_x]) rotate([0,90,0])
        keystone_hole();
}

// -----------------------------------------------------------------------------
// patchbox
// -----------------------------------------------------------------------------

module patchbox_body(with_supports=1) {
    translate([10,0,0]) mirror([0,1,0]) din_rail_clip(z=clipz);
    difference(){
        frame();
        translate([0, 0, bfz+pcb_z]) {
            // CAN connector hole
            translate([0, ft+pcb_spc+can_dy-fhes, 0]) cube([ft, (2*c_jstxh_x/2)+2*fhes, c_jstxh_z+fhes]);
        }
        // keystone hole
        translate([patchbox_keystone_dx, oy-c_keystn_y, oz-tfz]) {
            keystone_hole_laying();
        }
        // pwr connector holes
        translate([ft + pcb_spc, oy-ft, oz-tfz]) {
            translate([patchbox_pwr_dx - fhes, 0, 0]) cube([c_kf2510_x+2*fhes, ft, c_kf2510_z+fhes]);
            translate([patchbox_pwr_dx + c_kf2510_x - fhes, 0, 0]) cube([c_kf2510_x+2*fhes, ft, c_kf2510_z+fhes]);
        }
        patchbox_top_lid();
        bottom_lid();
    }
    standoffs(fourth=false);
    // keystone bracket
    translate([ox - ft - c_keystn_z - 7, oy-c_keystn_y, oz-tfz]) {
        keystone_receptacle_laying();
    }
    // supports
    canh_y = (2*c_jstxh_x/2)+2*fhes;
    if(with_supports) {
        for(i=[0,0.5,1]) {
            translate([ox-ft-c_keystn_z-7, oy-c_keystn_y+1.5, 0]) {
                translate([i*(c_keystn_z-support_width),0,0]) cube([support_width, c_keystn_y-1.5, bfz+pcb_z]);
            }
        }
        translate([0, 0, bfz+pcb_z]) {
            for(i=[0.33,0.66]) {
                // CAN connector hole
                translate([0, ft+pcb_spc+can_dy-fhes + i*canh_y, 0]) cube([ft, support_width, c_jstxh_z+fhes]);
                // PWR hole
                translate([ft + patchbox_pwr_dx + i*2*(c_kf2510_x+2*fhes), oy-ft, 0]) cube([support_width, ft, c_kf2510_z+fhes]);
            }
            for(i=[0.5]) {
                // keystone
                translate([ox-ft-c_keystn_z-7, oy-c_keystn_y+1.5, 0]) {
                    translate([i*(c_keystn_z-support_width),0,0]) cube([support_width, c_keystn_y-1.5, 17]);
                }
            }
        }
    }
}

module patchbox_top_lid() {
    difference() {
        translate([ox,0,oz+tlz]) rotate([0,180,0]) lid();
        translate([0, 0, bfz+pcb_z]) {
            // keystone hole
            /*
            translate([patchbox_keystone_dx, oy-c_keystn_y, 0]) {
                keystone_hole_laying();
                translate([0,oy-lid_frame_t,0]) cube([50, lid_frame_t, lid_frame_z]);
            }
            */
        }
        // keystone hole
        translate([patchbox_keystone_dx-lid_frame_hole_margin, oy-ft-lid_spc-lid_frame_t, oz-lid_frame_z-1])
            cube([c_keystn_z+2*lid_frame_hole_margin, ft+lid_spc+lid_frame_t, lid_frame_z+1]);
    }
}

// -----------------------------------------------------------------------------
// universal holder
// -----------------------------------------------------------------------------

module simple_holder(screws_distance, thickness=10, hole=3) {
    z = thickness;
    fn = 20;
    clipx = 42;

    // pad dimensions
    px = z;
    py = 3;

    // body dimensions
    bx = screws_distance - px;
    by = 5;

    // screw hole dimensions
    shr = hole/2;
    shh = py+by;

    x = screws_distance + px;

    // clip position
    clipdx = (x - clipx) / 2;

    translate([clipdx, 0, z]) rotate([180, 0, 0]) din_rail_clip(z=z);
    difference() {
        // body
        union() {
            cube([x, by, z]);
            translate([0, by, 0]) {
                cube([px, py, z]);
                translate([screws_distance, 0, 0]) cube([px, py, z]);
            }
        }
        // screw holes
        translate([px/2, shh, z/2]) {
            rotate([90, 0, 0]) cylinder(r=shr, h=shh, $fn=fn);
            translate([screws_distance, 0, 0]) rotate([90, 0, 0]) cylinder(r=shr, h=shh, $fn=fn);
        }
    }

}

color("red") render() {
    if(show_cpu_top_lid) cpu_top_lid();
    if(show_patchbox_top_lid) patchbox_top_lid();
    if(show_bpio_top_lid) top_lid();
    if(show_bottom_lid) bottom_lid();
}

// for printing
if(make_patchbox_body) {
    render() patchbox_body();
}
if(make_patchbox_top_lid) {
    render() translate([ox, 0, oz + tlz]) rotate([0,180,0]) patchbox_top_lid();
}
if(make_cpu_body) {
    render() cpu_body();
}
if(make_bpio_body) {
    render() bpio_body(with_supports=1);
}
if(make_bpio_top_lid) {
    render() translate([ox, 0, oz + tlz]) rotate([0,180,0]) top_lid();
}
if(make_4relays_holders) {
    render() simple_holder(45, thickness=10, hole=3);
    render() translate([0, 20, 0]) mirror([0, 1, 0]) simple_holder(45, thickness=10, hole=3);
}
if(make_cpu_top_lid) {
    render() translate([ox, 0, oz + tlz]) rotate([0,180,0]) cpu_top_lid();
}
if(make_bottom_lid) {
    render() bottom_lid();
}

//// PCBs
if(show_patchbox_pcb) {
    translate([ft+pcb_spc, ft+pcb_spc, bfz]) {
        color("green") cube([pcb_x, pcb_y, pcb_z]);
        color("blue") translate([0,0,pcb_z]) {
            // CAN
            translate([-1.64, can_dy, 0]) cube([c_jstxh_y, c_jstxh_x, c_jstxh_z]);
            // pwr
            translate([patchbox_pwr_dx, pcb_y-c_kf2510_y-1, 0]) cube([c_kf2510_x, c_kf2510_y, c_kf2510_z]);
            translate([patchbox_pwr_dx + c_kf2510_x, pcb_y-c_kf2510_y-1, 0]) cube([c_kf2510_x, c_kf2510_y, c_kf2510_z]);
        }
    }
}

if(show_bpio_pcb) {
    translate([ft+pcb_spc, ft+pcb_spc, bfz]) {
        color("green") cube([pcb_x, pcb_y, pcb_z]);
        color("blue") translate([0,0,pcb_z]) {
            // IDC
            translate([bpio_idc_dx, pcb_y-c_idc2x5_y+2.54, 0]) cube([c_idc2x5_x, c_idc2x5_y, c_idc2x5_z]);
            // CAN
            translate([-1.64, can_dy, 0]) cube([c_jstxh_y, 2*c_jstxh_x, c_jstxh_z]);
            // USB
            if(0) translate([pcb_x - 5, bpio_usb_dy, bpio_usb_dz]) cube([10, c_usb_y, c_usb_z]);
        }
    }
}

if(show_cpu_pcb) {
    translate([ft+pcb_spc, ft+pcb_spc, bfz]) {
        color("green") cube([pcb_x, pcb_y, pcb_z]);
        color("blue") translate([0,0,pcb_z]) {
            // CAN
            translate([-1.64, can_dy, 0]) cube([c_jstxh_y, 2*c_jstxh_x, c_jstxh_z]);
            // USB
            translate([pcb_x - 5, cpu_usb_dy, c_usb_dz]) cube([10, c_usb_y, c_usb_z]);
            // LEDs
            translate([cpu_leds2x2_dx, pcb_y-c_leds2x2_x, 0]) cube([c_leds2x2_x, c_leds2x2_x, c_leds2x2_z]);
        }
    }
}

if(show_cpu_lid_lock_test) {
    difference() {
        union() {
            render() cpu_body();
            color("red") render() cpu_top_lid();
        }
        //cube([ox/2,oy,oz+tlz]);
        //cube([ox,oy/2,oz+tlz]);

        render() translate([ox/2,0,0]) cube([ox/2,oy,oz+tlz]);
        render() cube([ox,oy/2,oz+tlz]);
    }
}
