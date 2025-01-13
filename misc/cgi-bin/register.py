#!/usr/bin/python3

import cgi
import os
import sys

os.set_blocking(sys.stdin.fileno(), False)
print("HTTP/1.1 200 OK")
print("Content-type: text/html\r\n\r\n")
form = cgi.FieldStorage()

username = form.getvalue("username")
password = form.getvalue("password")

# File storing user data
user_data_file = "../cgi-bin/tmp/users.txt"

# Ensure the tmp directory exists
os.makedirs("../cgi-bin/tmp", exist_ok=True)

# Check if the user already exists
if os.path.exists(user_data_file):
    with open(user_data_file, "r") as f:
        users = f.readlines()
        for user in users:
            stored_username, _ = user.strip().split(",")
            if username == stored_username:
                print(f"<h1>Registration failed!</h1>")
                print(f"<p>Username '{username}' is already taken. <a href='../html/register.html'>Try again</a>.</p>")
                exit()

# Register the new user
with open(user_data_file, "a") as f:
    f.write(f"{username},{password}\n")

print("<h1>Registration successful!</h1>")
print("<p>You can now <a href='../cgi-bin/login.py'>login</a>.</p>")
