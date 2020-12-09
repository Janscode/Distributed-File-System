//todo: import standard io, networking, and multithreading libraries

//todo: write thread to serve get request
    //todo: parse for file name
    //todo ec: report what chunks of file are here
    //todo: send back chunks, use 1028 byte segments labelled with chunk # (1 byte)
        //todo: ec: wait for requests for chunks
    //todo: send back transmission complete byte (invalid chunk #, 5 in this case)

//todo: write thread to serve put request
    //todo: parse for file name
    //todo: receive chunks, use 1028 byte segments labelled with chunk #
        //todo: if new chunk, construct chunk file name with '.' prefix and '.#' suffix
        //todo: receive transmission complete byte to break out of loop, or wait for socket timeout

//todo: write thread to serve list request
    //todo: use ls command with syscall
    //todo: report back file names with chunk #'s format: filename/##filename/##filename/## (numbers are single bytes, no delimiter)
    // send / in lieu of filename to mark the end of transmission


//todo: write main thread routine to serve all requests
    //todo: parse socket stream for request type, username, password
    //todo: check username and password
        //todo: if username is valid but no folder exists, create folder
        //todo: if username is invalid, send back error signal
    //todo: call appropriate handler, passing request details

//todo: write main
    //todo: read config file
        //todo: create username password data structure (linked list?) alternativley, just parse through config file each time
            //practice: hash and salt passwords
            //practice: implement a more efficient username lookup
        //todo: open socket on correct port
    //todo: while loop to listen to socket using select
        //todo: accept connection and spawn new thread
