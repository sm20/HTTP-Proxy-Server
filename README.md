# HTTP-Proxy-Server  


A  c-language proxy server for both receiving a client's(browser- sending HTTP requests) requests, forwarding that request to the actual server, receiving the response from the server, and finally forwarding that response to the original client.  


This program initializes the sockets needed for the communication. It then takes client requests and forwards server responses manipulated. It manipulates some text on sites, and images shown, both embedded and linked.


Usage:

    1.Set your browser(firefox) to read from proxy: port 8080 and localhost IP address 127.0.0.1
        Alternatively you can set your system to go through that proxy in network settings.
    2. compile using: gcc proxy.c -o proxy
    3. Go to a HTTP website
    4. Run: ./proxy in the console
    5. Interact with different elements on the site.
