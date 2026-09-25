# Metadata

Shairport Sync can deliver metadata supplied by the source, such as Album Name, Artist Name, Cover Art, etc.
through a pipe or UDP socket to a recipient application program — see https://github.com/mikebrady/shairport-sync-metadata-reader for a sample recipient.
Sources that supply metadata include iTunes and the Music app in macOS and iOS.

## Metadata Build Options

At build time, three configure options control metadata support:

- `--with-metadata`
	- Purpose: enable metadata support in general.
	- Effect: enables core metadata handling and also includes metadata pipe support.
	- Typical use: convenient default choice when you want metadata and Unix pipe output.

- `--with-metadata-pipe`
	- Purpose: add support for exporting metadata through a Unix pipe.
	- Effect: enables pipe-based metadata output only (unless other metadata outputs are also enabled).
	- Typical use: local integrations where another process reads metadata from `metadata.pipe_name`.

- `--with-metadata-multicast`
	- Purpose: add support for exporting metadata via UDP (including multicast addresses).
	- Effect: enables socket-based metadata output; use `metadata.socket_address` and `metadata.socket_port` in the configuration.
	- Typical use: send metadata to remote hosts or multiple listeners.

Notes:

- `--with-metadata` is effectively a convenience option that includes pipe metadata support.
- `--with-metadata-pipe` and `--with-metadata-multicast` can be combined; if both are compiled in, metadata can be sent to both outputs.
- UDP output has packet size limits; large items such as cover art may need `metadata.socket_msglength` tuning and can still exceed practical network limits.

### Example Configure Commands

Common metadata build setups:

```sh
# Pipe-only metadata output
./configure --with-metadata-pipe

# Multicast/UDP-only metadata output
./configure --with-metadata-multicast

# Pipe + multicast/UDP metadata output
./configure --with-metadata-pipe --with-metadata-multicast
```


## Metadata over UDP

As an alternative to sending metadata to a pipe, the `socket_address` and `socket_port` tags may be set in the metadata group to cause Shairport Sync
to broadcast UDP packets containing the track metadata.

The advantage of UDP is that packets can be sent to a single listener or, if a multicast address is used, to multiple listeners.
It also allows metadata to be routed to a different host. However UDP has a maximum packet size of about 65000 bytes; while large enough for most data, Cover Art will often exceed this value. Any metadata exceeding this limit will not be sent over the socket interface. The maximum packet size may be set with the `socket_msglength` tag to any value between 500 and 65000 to control this - lower values may be used to ensure that each UDP packet is sent in a single network frame. The default is 500. Other than this restriction, metadata sent over the socket interface is identical to metadata sent over the pipe interface.

The UDP metadata format is very simple - the first four bytes are the metadata *type*, and the next four bytes are the metadata *code*
(both are sent in network byte order - see https://github.com/mikebrady/shairport-sync-metadata-reader for a definition of those terms).
The remaining bytes of the packet, if any, make up the raw value of the metadata.

