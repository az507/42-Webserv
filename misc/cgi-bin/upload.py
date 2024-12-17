-----------------------------140852920241981611193399813780
Content-Disposition: form-data; name="uploaded_file"; filename="time.py"
Content-Type: text/x-python

#!/usr/bin/python3

import datetime
import cgi

print("HTTP/1.1 200 OK")
print("Content-type: text/html\r\n\r\n")
print("<html>")
print("<head>")
print(datetime.datetime.strftime(datetime.datetime.now(), "<h1>  %H:%M:%S </h1>"))
-----------------------------140852920241981611193399813780--
