#!/usr/bin/python3

import cgi, os, sys, cgitb

print("HTTP/1.1 200 OK")
cgitb.enable()
#os.set_blocking(sys.stdin.fileno(), False)
form = cgi.FieldStorage()

print("Content-Type: text/html")
print()

# Get filename here
fileitem = form['filename']

# Test if the file was uploaded
if fileitem.filename:
    message = "is ok"
    path = os.getenv("UPLOAD_DIR", "tmp/")
    if len(path) > 1 and path[-1] != '/':
        path += '/'
    os.makedirs(path, exist_ok=True)
    open(path + os.path.basename(fileitem.filename), 'wb').write(fileitem.file.read())
    #open(os.getcwd() + '/misc/cgi-bin/tmp/' + os.path.basename(fileitem.filename), 'wb').write(fileitem.file.read())
    #message = 'The file "' + os.path.basename(fileitem.filename) + '" was uploaded to ' + os.getcwd() + '/cgi-bin/tmp'
else:
    message = "not ok"
    #message = 'Uploading Failed'

print("<html>")
print("<head>")
print("<title>Upload Result Page</title>")
print("<H1> " + message + " </H1>")
print("<p> File uploaded as " + os.getcwd() + "/" + path + os.path.basename(fileitem.filename) + " </p>")
print("</head>")
print("</html>")
