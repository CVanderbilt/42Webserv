#!/usr/bin/env perl

local ($buffer, @pairs, $pair, $name, $value, %FORM);
# Read in text
$ENV{'REQUEST_METHOD'} =~ tr/a-z/A-Z/;
if ($ENV{'REQUEST_METHOD'} eq "POST") {
	read(STDIN, $buffer, $ENV{'CONTENT_LENGTH'});
} else {
	$buffer = $ENV{'QUERY_STRING'};
}

# Split information into name/value pairs
@pairs = split(/&/, $buffer);
foreach $pair (@pairs) {
	($name, $value) = split(/=/, $pair);
	$value =~ tr/+/ /;
	$value =~ s/%(..)/pack("C", hex($1))/eg;
	$FORM{$name} = $value;
}

$first_name = $FORM{fname};
$last_name = $FORM{lname};

print "Set-Cookie: prueba=valor\n";
print "Content-Type: text/html\r\n\r\n";
print "<html>";
print "<head>";
print "<title>Hello Perl CGI</title>";
print "</head>";
print "<body>";
print "<h2>Hello $first_name $last_name</h2>";
print "</body>";
print "</html>";

1;
