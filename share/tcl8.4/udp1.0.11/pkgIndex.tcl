# Handcrafted pkgIndex for tcludp.
if {[info exists ::tcl_platform(debug)]} {
    package ifneeded udp 1.0.11 [list load [file join $dir udp1011g.dll]]
} else {
    package ifneeded udp 1.0.11 [list load [file join $dir udp1011.dll]]
}
