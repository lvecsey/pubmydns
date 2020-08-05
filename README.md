
Run pubmydns from a tcpserver to collect IP addresses.
You basically want to collect IP addresses from machines with interfaces that
have public addresses on them, i.e. an interface with a dhcp client.

*Setting up the client machines*

On those machines, put in a crontab entry.

```console
*/7 * * * *	tcpclient -vRHl0 server_ip 5930 ~/bin/pubmydns p4p1 0
```

Where the server_ip is replaced and default port 5930 are utilized (TCP)
and the pubmydns binary is in place along with the public interface with
public ip to be checked.

The zero represents the index number to save into. So for example if you are the first client you could use 0, the next client would use 1, and so on. These are just unique identifiers, and could possibly be a hostname in a future version of this system.

*Setting up the server machine(s)*

The server itself can be started with tcpserver (this can be a run file):

```console    
export PUBMYDNS_MMAPFN='/tmp/pubmydns.mmap'
tcpserver -vRHl0 0.0.0.0 5930 ~/bin/pubmydns
```

For example, you might want to create a daemontools service called pubmydns_server:

```console
sudo mkdir -p /etc/service/.pubmydns_server
# be sure to create an executable run file in above directory, and test it.
sudo mv /etc/service/.pubmydns_server /etc/service/pubmydns_server
```

You can create a reasonably sized pubmydns.mmap file first.

```console
dd if=/dev/zero of=/tmp/pubmydns.mmap count=1 bs=4096
chmod 600 /tmp/pubmydns.mmap
```

The file simply represents 32byte records where the first 16 bytes of each record are the index number, and the following 16bytes contain the ip address.

*Reading the pubmydns.mmap file*

Just run

```console
~/bin/pubmydns show /tmp/pubmydns.mmap
```

It will give output such as the following:

```
0->ip.ad.re.ss
1->ip.ad.re.ss
...
```

for each client that has contributed an ip address.

*Other ideas*

Use rsync+ssh to replicate the mmap file to other servers, including a backup server. Probably, your servers already have ssh running on them so you can always rsync+ssh the .mmap file to a local machine to run pubmydns show on them or remotely log into them to view the current state of the mmap file.

