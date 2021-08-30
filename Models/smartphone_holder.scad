$fn=50;

// Dimensiones
L = 40; // Largo del pie
H = 80; // Altura
W = 7.5; // Ancho
R = 7; // Radio curvatura angulo
// Orificios para tornillos
sep = 16; // Separacion
db = 7.5; // Distancia al borde del pie
// Largo y alto a quitar para el ahuecamiento central
L2 = L*0.82; 
H2 = H-4; 
// Dimensiones rectangulo diagonal
w = L-L2-H+H2; 
s = sqrt(H*H+w*w); 

module rem(){
    union(){ // Recorte interior
        translate([(L-L2+R)/2,0,(H-H2)/2])
            cube([(L2-R),W,H2], center = true);
        
        translate([(L-L2)/2,0,(H-H2+R)/2])
            cube([L2,W,H2-R], center = true);
        
        translate([L/2-L2+R,0,H/2-H2+R])
        rotate([0,90,90])
            cylinder(r = R, h = W+1, center = true);
    }
    
    // Recorte diagonal
    translate([-L/2,-W/2,-H/2])
    rotate([0,-asin(H/s),0])
        cube([s,W,w*H/s]);
}

module holes(){
        
    Rh1 = 2.25; // Radio para cabeza del tornillo
    Rh2 = 1.8; // Radio para rosca del tornillo
    ph = 1.5; // Espesor de la zona de ajuste

    translate([L/2-db,0,-H2/2+ph/2])
        cylinder(r = Rh1, h = H-H2-ph, center = true);
    translate([L/2-db,0,-H2/2])
        cylinder(r = Rh2, h = H-H2, center = true);
    
    translate([L/2-db-sep,0,-H2/2+ph/2])
        cylinder(r = Rh1, h = H-H2-ph, center = true);
    translate([L/2-db-sep,0,-H2/2])
        cylinder(r = Rh2, h = H-H2, center = true);
}


difference(){
    cube([L,W,H], center = true);
    rem();
    holes();
}