#/usr/bin/perl -w 

$| = 1; # autoflush STDOUT

$i = 0;
while (true) {
  print "$i ";
  $i = $i + 1;
  sleep(1);
}
