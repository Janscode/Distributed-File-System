//todo: import standard io, networking, (maybe) multithreading, and hashing

//todo: write put request
    //todo: write put request main thread
        //todo: read file in chunks
            //todo: error if the file isn't found
            //todo: progressivley update state of md5 object
            //todo: progressivley count file length
        //todo: compute md5 hash, figure out partition strategy
        //todo: save each chunk of the file locally ec: encrypt chunks with simple encryption (xor password for now)
        //todo: for each server, intiate connection, get ok back, send appropriate chunks according to partition strategy
        //todo: delete local files

    //practice: write put request multi thread routine (one thread for each server)

//todo: write get request
    //todo: write get request main thread
         //todo: for each server, intiate connection, get ok back
            //ec: check if the chunks are still needed, request if they are
            //todo: save server chunks locally
        //todo: reassemble chunks into main file ec: decrypt chunks
        //todo: delete local files

    //practice: write get request multi thread routine

//todo: write list request
    //todo: write list request main thread
    
    //practice: write list request multi thread routine

//todo: write main
    //todo: read dfc.config
        //todo: store server information / build address structs
        //todo: store username / password info or request it if it doesn't exist
    //todo: main input loop
        //todo: parse request kind and call appropriate function
