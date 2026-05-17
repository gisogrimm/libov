# libov

'libov' is part of the OVBOX system.  The repository contains the
header and library files needed to communicate with the audio relay
server protocol ('Room Service') and to communicate with the
configuration server via a REST API. More information can be found at
[ovbox.de](https://ovbox.de/) or
[github.com/gisogrimm/ov-client](https://github.com/gisogrimm/ov-client). The
code of the configuration server can be found at
[github.com/gisogrimm/ov-webfrontend](https://github.com/gisogrimm/ov-webfrontend).


## Operation modes

```
PEER2PEER
RECEIVEDOWNMIX_deprecated
DONOTSEND
SENDDOWNMIX_deprecated
USINGPROXY
ENCRYPTION
```

| self      | self      | peer      | peer       | action         |
|-----------|-----------|-----------|------------|----------------|
| PEER2PEER | is_proxy? | PEER2PEER | use proxy? |                |
|-----------|-----------|-----------|------------|----------------|
| true      |           | true      |            | send to peer   |
| false     |           | false     |            | send to server |
| true      |           | false     |            | send to server |
| false     |           | false     |            | send to server |

