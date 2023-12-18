$fn = 100;

// Adapter dimensions
Ro = 7.5;
Ri = 2.6;
L = 35;
Lc = 17;
Hc = 3.2;
Ht = 2.9;

// Holder and base dimensions
Wb = 12.5; // Base width
Lb = 60; // Base length
Hbb = 5; // Base height
Hbt = 65; // Holder height (mounted on base)
rh = 2; // Hole radius
lh = 5; // Hole length (from circles centers)
Sh = 40; // Holes separation



module half(){
    translate([L/2, 0, 0])
        difference(){
            union(){
                cylinder(h = Ht, r = Ro, center = true);
                translate([-L/4, 0, 0])
                    cube([L/2, Ro*2, Ht], center = true);
            }
            cylinder(h = Ht, r = Ri, center = true);
        }
}

module center(){
    translate([Lc/4, 0, Hc/2])
        cube([Lc/2, Ro*2, Hc], center = true);
}


module adapter() {
    translate([0,0,(Hc+Ht)/2]) half();
    translate([0,0,-(Hc+Ht)/2]) half();
    translate([0,0,-Hc/2]) center();
}

module screw_hole() {
    linear_extrude(height = Hbb, center = true, convexity = 10, twist = 0)
    hull() {
        translate([0,lh/2,0]) circle(rh);
        translate([0,-lh/2,0]) circle(rh);
    }
}
    

module base() {
    union() { // Base and holder
        difference() { // Base with holes
            cube([Wb, Lb, Hbb], center = true);
            translate([0, Sh/2, 0]) screw_hole();
            translate([0, -Sh/2, 0]) screw_hole();
        }
        translate([0, 0, (Hbb+Hbt)/2])
            cube([Wb, 3*Ro, Hbt], center = true);
    }
}

module holder() {
    base();
    translate([Wb/2,0,Hbb/2+Hbt-Ht-Hc/2]) adapter();
}

holder();
