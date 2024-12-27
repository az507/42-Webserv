#!/usr/bin/python3

import os
import json

# Include a proper HTTP status line and headers
print("HTTP/1.1 200 OK")  # Ensure HTTP/1.1 protocol response
print("Content-type: application/json")
print()  # Blank line to separate headers from the body

try:
    tmp_dir = os.getcwd() + "/tmp"
    #tmp_dir = "/home/achak/Documents/webserv/misc/cgi-bin/tmp"
    files = [f for f in os.listdir(tmp_dir) if os.path.isfile(os.path.join(tmp_dir, f))]
    
    # Output valid JSON
    print(json.dumps(files), end="")
except Exception as e:
    print(json.dumps({"error": str(e)}), end="")
