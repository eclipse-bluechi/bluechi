<!-- markdownlint-disable-file MD013 MD033 -->
# Network

The BlueChi controller is exposed on TCP port 842, making it subject to attacks from malacious actors within the network. However, similar to other services, protective measures can be employed to defend it. By configuring firewall rules within the system, VLANs or network devices.

As example, let's see Distributed Denial of Service (DDoS) attacks scenario. These attacks can overwhelm its infrastructure by flooding it with an immense volume of traffic, disrupting normal operations and potentially causing significant downtime.

Below some examples to defend it using firewalld and iptables:

## Setup firewall rules

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

Additionally, users can also utilize iptables to block connections from any IP address that attempts excessive connections, such as reaching a count of **1000**.

``` bash
sudo iptables -A INPUT -p tcp --dport 842 -m conntrack --ctstate NEW -m recent --name BLUECHIRULE --set

sudo iptables -A INPUT -p tcp --dport 842 -m conntrack --ctstate NEW -m recent --name BLUECHIRULE --update --seconds 60 --hitcount 5 -j DROP
```

## Securing connections

Briefly mentioning that the communication between the BlueChi components is, by default, not secured and that the getting started guide provides an possible approach using a double proxy to achieve that.

For more information, [see our documentation page](../getting_started/securing_multi_node.md).
