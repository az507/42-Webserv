#!/usr/bin/python3

import cgi
import sys
import os

os.set_blocking(sys.stdin.fileno(), False)
print("HTTP/1.1 200 OK")
print("Content-type: text/html\r\n\r\n")
form = cgi.FieldStorage()

username = form.getvalue("username")
password = form.getvalue("password")

# File storing user data
user_data_file = "../cgi-bin/tmp/users.txt"

# Check if the user exists
if os.path.exists(user_data_file):
    with open(user_data_file, "r") as f:
        users = f.readlines()
        for user in users:
            stored_username, stored_password = user.strip().split(",")
            if username == stored_username and password == stored_password:
                print(f"<h1>Welcome, {username}!</h1>")
                exit()

print("<h1>Login failed!</h1>")
print("<p>Invalid username or password. <a href='/html/login.html'>Try again</a>.</p>")
