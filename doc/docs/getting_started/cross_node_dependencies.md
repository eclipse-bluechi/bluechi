<!-- markdownlint-disable-file MD010 MD013 MD014 MD024 MD046 -->

# Resolving cross-node dependencies

In practice, it is a common scenario that a service running on Node A requires another service to run on Node B. Consider the following example:

![BlueChi cross-node dependency example](../assets/img/bluechi_cross_node_dependency.png)

The overall service consists of two applications - a webservice rendering an HTML and a CDN providing assets such as images.

The following sections will provide a simplistic implementation of this example. In addition, it is assumed that a running CDN service on the raspberry pi is required by the webservice on the laptop. This cross-node dependency is then resolved by employing BlueChi's proxy service feature - so starting the webservice will also start the CDN.

!!! Note

    A detailed explanation of how Proxy Services are implemented in BlueChi based on systemd can be found [here](../cross_node_dependencies/index.md).

## Setting up the CDN

On the raspberry pi, first create a temporary directory and add an example image to it:

```bash
mkdir -p /tmp/bluechi-cdn
cd /tmp/bluechi-cdn
wget https://raw.githubusercontent.com/eclipse-bluechi/bluechi/main/doc/docs/bluechi_architecture.jpg
```

For the sake of simplicity, python's [http.server module](https://docs.python.org/3/library/http.server.html) is used to simulate a CDN. Create a new file `/etc/systemd/system/bluechi-cdn.service` and paste the following systemd unit definition:

```systemd
[Unit]
Description=BlueChi's CDN from the cross-node dependency example

[Service]
Type=simple
ExecStart=/usr/bin/python3 -m http.server 9000 --directory /tmp/bluechi-cdn/
```

Reload the systemd controller configuration so it can find the newly created service:

```bash
$ systemctl daemon-reload
```

Let's verify that the service works as expected. First, start the service via:

```bash
$ systemctl start bluechi-cdn.service
```

Then submit a query to it. The response should be an HTML page that contains the previously added image as a list item:

```html
$ curl localhost:9000

<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 4.01//EN" "http://www.w3.org/TR/html4/strict.dtd">
<html>
  <head>
    <meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
    <title>Directory listing for /</title>
  </head>
  <body>
    <h1>Directory listing for /</h1>
    <hr />
    <ul>
      <li><a href="bluechi_architecture.jpg">bluechi_architecture.jpg</a></li>
    </ul>
    <hr />
  </body>
</html>
```

Finally, stop the service again:

```bash
$ systemctl stop bluechi-cdn.service
```

## Setting up the WebService

On the laptop, also create a temporary directory as well as an empty python file:

```bash
mkdir -p /tmp/bluechi-webservice
cd /tmp/bluechi-webservice
touch service.py
```

The following code is a rudimentary implementation of the webservice which should be able to run without any other dependencies. On any request, it will return a small HTML string containing a link to the image in the CDN. Copy and paste it into the previously created `service.py`.

```python
import sys
import wsgiref.simple_server

port: int = None
cdn_host: str = None

def service(environ, start_response):
    status = "200 OK"
    headers = [("Content-Type", "text/html")]

    response = f"""
<html>
    <body>
        <img src="http://{cdn_host}/bluechi_architecture.jpg" alt="bluechi_architecture" />
    </body>
</html>
    """

    start_response(status, headers)
    return [response.encode("utf-8")]

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python3 webservice.py <port> <cdn-host>")
        exit(1)

    port = int(sys.argv[1])
    cdn_host = sys.argv[2]

    server = wsgiref.simple_server.make_server(
        host="localhost",
        port=port,
        app=service
    )

    while True:
        server.handle_request()
```

Let's create a new systemd unit to wrap the `service.py`. Copy and paste the following unit definition into `/etc/systemd/system/bluechi-webservice.service`:

```systemd
[Unit]
Description=BlueChi's webservice from the cross-node dependency example

[Service]
Type=simple
ExecStart=/usr/bin/python3 /tmp/bluechi-webservice/service.py 9000 192.168.178.55:9000
```

!!! Note

    Make sure to replace the IP address `192.168.178.55` with the corresponding one of the raspberry pi in your setup.

Start the service via `systemctl` to verify it works as expected:

```html
$ systemctl start bluechi-webservice.service $ curl localhost:9000

<html>
  <body>
    <img
      src="http://192.168.178.55:9000/bluechi_architecture.jpg"
      alt="bluechi_architecture"
    />
  </body>
</html>
```

Make sure to stop the service again before proceeding:

```bash
$ systemctl stop bluechi-webservice.service
```

## Resolving the dependency via BlueChi

After implementing both applications, let's ensure that starting the webservice will also start the CDN. Add the following lines to `bluechi-webservice.service` in the `[Unit]` section:

```systemd
Wants=bluechi-proxy@pi_bluechi-cdn.service
After=bluechi-proxy@pi_bluechi-cdn.service
```

`bluechi-proxy@.service` is a [systemd service template](https://www.freedesktop.org/software/systemd/man/systemd.service.html#Service%20Templates) provided by BlueChi that enables resolving cross-node dependencies. The argument between the `@` symbol and the `.service` postfix follows the pattern `bluechi-proxy@<node>_<service>.service`. BlueChi will split the argument accordingly and start the specified `<service>` on the `<node>`. In the case above, it will start `bluchi-cdn.service` on the node `pi`.

The systemd unit mechanism [Wants](https://www.freedesktop.org/software/systemd/man/systemd.unit.html#Wants=) is used for declaring that dependency and starting it (if not already running). In addition, the [After](https://www.freedesktop.org/software/systemd/man/systemd.unit.html#Before=) setting is used to ensure that the webservice starts only after the CDN has been started.

The final `bluechi-webservice.service` should look like this:

```bash
$ cat /etc/systemd/system/bluechi-webservice.service

[Unit]
Description=BlueChi's webservice from the cross-node dependency example
Wants=bluechi-proxy@pi_bluechi-cdn.service
After=bluechi-proxy@pi_bluechi-cdn.service

[Service]
Type=simple
ExecStart=/usr/bin/python3 /tmp/bluechi-webservice/service.py 9000 192.168.178.55:9000
```

Since the dependency is encoded into the systemd unit, the webservice can be started either via `bluechictl` or `systemctl`:

```bash
bluechictl start laptop bluechi-webservice.service
# or use
systemctl start bluechi-webservice.service
```

On the laptop - where `bluechi-controller` is running - use `bluechictl` to verify that both services are running now:

```bash
$ bluechictl status laptop bluechi-webservice.service

UNIT                            | LOADED        | ACTIVE        | SUBSTATE      | FREEZERSTATE  | ENABLED       |
----------------------------------------------------------------------------------------------------------------
bluechi-webservice.service      | loaded        | active        | running       | running       | static        |

$ bluechictl status pi bluechi-cdn.service

UNIT                    | LOADED        | ACTIVE        | SUBSTATE      | FREEZERSTATE  | ENABLED       |
--------------------------------------------------------------------------------------------------------
bluechi-cdn.service     | loaded        | active        | running       | running       | static        |
```

Use a webbrowser on the laptop and navigate to `localhost:9000`. The resulting page is served by the `bluechi-webservice.service` (laptop) and it displays the image fetched from the `bluechi-cdn.service` (raspberry pi).
