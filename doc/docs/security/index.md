<!-- markdownlint-disable-file MD013-->
# Security

As a distributed system, BlueChi is subject to attacks from malicious actors within the network.
However, similar to other services, protective measures can be employed to defend against such scenarios. By setting up [firewall rules](#setup-firewall-rules) and [IP address filter](#setup-ip-address-filter) the attack surface can be reduced. The security of the system can be enhanced even further by leveraging [BlueChi's SELinux policy](./selinux.md) as well as setting up a [double proxy for mTLS encryption](./securing_multi_node.md).

## Setup firewall rules

By using [firewalld](https://firewalld.org/), rules can be defined to secure BlueChi and the overall system:

``` bash
# Install firewalld
$ sudo dnf install firewalld

# Enable and start the service
$ sudo systemctl enable --now firewalld

# Allow communication on port 842/tcp
$ sudo firewall-cmd --permanent --zone=public --add-port=842/tcp

# Block a specific IP address from a malicious agent
$ sudo firewall-cmd --permanent --zone=public --add-rich-rule='rule family="ipv4" source address="ADD_HERE_MALICIOUS_IP_FROM_BAD_ACTOR" port port="842" protocol="tcp" drop'

# Reload firewalld with the new configuration
$ sudo firewall-cmd --reload
```

## Setup IP address filter

Additionally, [iptables](https://linux.die.net/man/8/iptables) can be utilized to block connections from any IP address that attempts excessive connections:

``` bash
sudo iptables -A INPUT -p tcp --dport 842 -m conntrack --ctstate NEW -m recent --name BLUECHIRULE --set

sudo iptables -A INPUT -p tcp --dport 842 -m conntrack --ctstate NEW -m recent --name BLUECHIRULE --update --seconds 60 --hitcount 5 -j DROP
```
