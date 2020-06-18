# Post processing

EgilSCIM allows post processing of the SCIM objects before they are sent to the
server. You can develop your own post processing by implementing a plugin
as a dynamically loaded library (.so file on Linux or a .dll on Windows).

By implementing your own plugin you have free flexibility to convert, filter or
augment the data.

You will need to know some programming in order to implement your own plugin,
or you can choose to use one of the pre-written plugins from Föreningen
Sambruk.

## Using a plugin

For this example, assume there are two installed plugins: profile and
javascript. They are both installed in the directory `/usr/lib/egil/pp`.

In your master configuration file, add variables for finding the plugins:

```
post-process-plugin-path = /usr/lib/egil/pp
post-process-plugins = profile javascript
```

When EgilSCIM is about to send a JSON object to the SCIM server, it will
first send the JSON text through each plugin listed in `post-process-plugins`,
in the stated order.

## Implementing a plugin

For the required C interface of the plugin, see `src/pp_interface.h`.

There's an example plugin which simply copies the input text without making
any real post processing changes. You'll find it under `plugins/pp/echo`.
