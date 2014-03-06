## AgFS programs
AgFS has three distinct programs with the following options.
* agfs-client [-i *instance*] *mount point*
* agfs-server [-p *port*]
* agfs-keygen *dns-name/ipaddr* *user* *mount* *key-filename*

## Storage locations for AfFS data
### Server
The server stores all generated keys and the key mapping file in */var/lib/agfs*.
Keys are generated through the agfs-keygen program and are automatically put in key folder
and appended to the key mapping file.

### Client
The client stores its keys and its instance definitions in *~/.agfs/*. Keys must be manually
placed by the user after being recieved from a server administrator. Users may define instances
to limit what set of keys get used. 
