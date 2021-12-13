#!/usr/bin/env python2

# Import modules for CGI handling
import cgi, os

# Create instance of FieldStorage
form = cgi.FieldStorage()

# Get data from fields
fname = form.getvalue('fname')
lname = form.getvalue('lname')

print 'Content-type:text/html\r\n\r\n'
print '<html>'
print '<head>'
print '<title>Hello %s - CGI Program</title>' % (os.environ["REQUEST_METHOD"])
print '</head>'
print '<body>'
print '<h2>Hello %s %s</h2>' % (fname, lname)
print '</body>'
print '</html>'
