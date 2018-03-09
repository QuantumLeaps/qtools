# script to re-start a given test fixture repetitively so that
# a remote test script can reset the target.

puts "press ^C to quit"

while {1} {
    puts "(re)starting [lindex $::argv 0] [lindex $::argv 1] [lindex $::argv 2]"
    exec [lindex $::argv 0] [lindex $::argv 1] [lindex $::argv 2]
}
