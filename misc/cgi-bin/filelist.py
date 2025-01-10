#!/usr/bin/python3

import os

# Define the directory to list
directory = "/misc/cgi-bin/tmp"

print("Content-type: text/html\r\n\r\n")
print("<html>")
print("<head><title>File Deletion</title></head>")
print("<body>")
print("<h1>Select a File to Delete</h1>")

try:
    # List files in the directory
    files = os.listdir(directory)
    if not files:
        print("<p>No files available to delete.</p>")
    else:
        print("<form id='deleteForm'>")
        print("<label for='fileSelect'>Choose a file:</label>")
        print("<select id='fileSelect' name='filename'>")
        for file in files:
            print(f"<option value='{file}'>{file}</option>")
        print("</select>")
        print("<button type='button' onclick='deleteFile()'>Delete</button>")
        print("</form>")
except Exception as e:
    print(f"<p>Error: {e}</p>")

print("""
<script>
function deleteFile() {
    const file = document.getElementById('fileSelect').value;
    if (confirm(`Are you sure you want to delete ${file}?`)) {
        fetch(`/misc/cgi-bin/tmp/` + file, {
            method: 'DELETE'
        }).then(response => {
            if (response.ok) {
                alert('File deleted successfully.');
                location.reload();
            } else {
                alert('Failed to delete the file. Please try again.');
            }
        }).catch(error => {
            console.error('Error:', error);
            alert('An error occurred.');
        });
    }
}
</script>
""")

print("</body>")
print("</html>")
