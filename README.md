## EC
I attempted the smart get request extra credit.
## Implementation
### General
For all communications, the code uses a request response like structure, so after any send, there is ususally a recv and vice versa.
The exception to the above is whenever a file is transfered, the first bit of the stream is used to indicate whether more bytes follow.
For all commands, an initial request with authenticaion details as well as the requested command and, if applicable, file name, is sent.
Each server is always contacted sequentially, one after another, with the entire communication sequence completing for each server before the next one.
### Put
First, the file is hashed and the size is recorded.
Then, the file is broken into chunks locally.
Next, each server is contacted sequentially, and once authenticated, the right chunks are sent over. There is a confirmation before each chunk is sent.
Finally, the chunks are deleted at the client side.
### Get
The client contacts each server sequentially.
Each server advertises it's chunks one after another, and the client either accepts or refuses the chunk with a single byte message.
If accepted, the server sends back the chunk, and the client records which chunk was received, as well as saving a local copy.
Once the server has sent all it's chunks back, the server sends a done message.
The client checks if all chunks are received, and either reassembles or informs the user of an error.
Finally, the chunks are deleted at the client side.
### List
The client contacts each server sequentially.
The server sends back the exact file names, 1 by 1, waiting for a confirmaiton of receipt by the client each time.
The server sends a message once it is finished.
The client creates a node in a linked list for any new filename, and marks an array within those nodes for each chunk.
The data structure used to keep track of chunks is the same as in get, except it is encapsulated in a linked list node.
Finally, the client traverses and progressivley consumes the linked list, checking for each file node whether the file is complete or incomplete.


## Usage
To compile everything, enter "make" into the command line.
To run each server, enter "./dfs \<dirname>/ \<portno> into the command line.
To run a client, enter "./dfc \<config file> into the command line.