#!/usr/bin/env python3
import cgi
import os

print("Content-Type: text/html\n")
print("<html><body>")
print("<h2>Debug: Script started.</h2>")

try:
    # Parse form data
    form = cgi.FieldStorage()
    print("<p>Debug: Form data received.</p>")

    if 'uploaded_file' not in form:
        raise ValueError("No file uploaded!")

    fileitem = form['uploaded_file']
    print(f"<p>Debug: Filename = {fileitem.filename}</p>")

    # Determine upload directory
    upload_dir = os.path.join(os.getcwd(), 'misc', 'cgi-bin', 'tmp')
    if not os.path.exists(upload_dir):
        os.makedirs(upload_dir)
        print(f"<p>Debug: Created directory {upload_dir}</p>")
    else:
        print(f"<p>Debug: Directory exists: {upload_dir}</p>")

    # Save file if it exists
    if fileitem.filename:
        filepath = os.path.join(upload_dir, os.path.basename(fileitem.filename))
        with open(filepath, 'wb') as f:
            f.write(fileitem.file.read())
        print(f"<p>File {fileitem.filename} uploaded successfully to {filepath}.</p>")
    else:
        print("<p>No file uploaded.</p>")

except Exception as e:
    print(f"<p>Error occurred: {e}</p>")

finally:
    print("<p>Debug: Script finished execution.</p>")
    print("</body></html>")
