# webserv
This project is about writing your own HTTP server. You will be able to test it with an actual browser. HTTP is one of the most used protocols on the internet. Knowing its arcane will be useful, even if you won’t be working on a website.



## HTTP response status codes<br>
> 200 ok sucesful response<br>
> 204 No Content request has succeeded, but the client doesn't need to navigate away from its current page.<br>
> 301 Moved Permanently requested resource has been permanently moved to the URL in the Location header.<br>
> 400 Bad Request that the server would not process the request due to something the server considered to be a client error<br>
> 403 Forbidden status code indicates that the server understood the request but refused to process it<br>
> 404 Not Found status code indicates that the server cannot find the requested resource<br>
> 405 Method Not Allowed tatus code indicates that the server knows the request method, but the target resource doesn't support this method. <br>
> 409 Conflict code indicates a request conflict with the current state of the target resource.<br>
> 413 Content Too Large he request entity was larger than limits defined by server.<br>
> 414 URI Too Long URI requested by the client was longer than the server is willing to interpret.<br>
> 500 Internal Server Error server encountered an unexpected condition that prevented it from fulfilling the request<br>                                                                                                      501 Not Implemented  server does not support the functionality required to fulfill the request.<br>

## Dependencies
For Running CGI scripts:
```bash
sudo apt-get update && sudo apt-get install -y zsh python3
```

## Usage
```bash
git clone https://github.com/mervintan76/webserv.git
cd webserv
make
./webserv # Listens on http://localhost:8080/
```
