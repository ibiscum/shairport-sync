# Running Shairport Sync as a system service with `systemd`

This guide shows how to install and run Shairport Sync as a **system-level** `systemd` service (managed by PID 1, started at boot).

## 1. Install the `systemd` development metadata (for `--with-systemd-startup`)

When you configure with `--with-systemd-startup`, the build system asks `pkg-config` for the `systemd` unit directory. On some distributions this requires `systemd-dev` (or equivalent).

On Debian / Ubuntu:

```bash
sudo apt update
sudo apt install --no-install-recommends systemd-dev
```

If your distribution uses a different package name, install the package that provides:

- `pkg-config` metadata for `systemd` (the `systemd.pc` file),
- and the unit directory variable `systemdsystemunitdir`.

## 2. Configure with `--with-systemd-startup`

From the Shairport Sync source directory:

```bash
autoreconf -fi
./configure --with-ssl=openssl --with-pkg-config --with-airplay-2 --with-alsa --with-avahi --with-systemd-startup
```

Notes:

- Add any other features you need (`--with-pipewire`, `--with-metadata`, etc.).
- If unit-directory auto-detection is not suitable, override it with:

```bash
./configure ... --with-systemd-startup --with-systemdsystemunitdir=/usr/lib/systemd/system
```

## 3. Build and install

```bash
make
sudo make install
```

With `--with-systemd-startup`, install places `shairport-sync.service` into the system unit directory (unless one already exists there).

## 4. Create and adjust the runtime configuration

The sample config file is usually installed as:

- `/etc/shairport-sync.conf.sample` (Linux),
- `/usr/local/etc/shairport-sync.conf.sample` (FreeBSD).

On Linux, create the active config if needed:

```bash
sudo cp /etc/shairport-sync.conf.sample /etc/shairport-sync.conf
sudo nano /etc/shairport-sync.conf
```

Set at least your desired output backend/device settings before first start.

## 5. Reload unit files and enable the service

```bash
sudo systemctl daemon-reload
sudo systemctl enable shairport-sync.service
sudo systemctl start shairport-sync.service
```

Check status:

```bash
systemctl status shairport-sync.service
```

View logs:

```bash
journalctl -u shairport-sync.service -f
```

## 6. Verify network advertisement and playback

1. Confirm the service is `active (running)` in `systemctl status`.
2. From an AirPlay source, verify `Shairport Sync` appears as an output device.
3. Start playback and confirm no startup errors appear in:
   - `journalctl -u shairport-sync.service`.

## 7. Service lifecycle commands

```bash
sudo systemctl restart shairport-sync.service
sudo systemctl stop shairport-sync.service
sudo systemctl disable shairport-sync.service
```

## 8. Troubleshooting checklist

- **Unit not found**  
  Run `sudo systemctl daemon-reload` and verify the installed unit path:
  ```bash
  pkg-config --variable=systemdsystemunitdir systemd
  ```

- **`./configure` fails around `systemd` / `pkg-config`**  
  Install `systemd-dev` (or your distro's equivalent package).

- **Service fails at startup**  
  Inspect:
  ```bash
  systemctl status shairport-sync.service
  journalctl -u shairport-sync.service -n 200 --no-pager
  ```
  Most failures are configuration/output-device issues in `shairport-sync.conf`.

- **Not visible in AirPlay sources**  
  Ensure mDNS is running (for Avahi builds), and check firewall/network segmentation.

## 9. Updating after rebuilding

After rebuilding and reinstalling:

```bash
sudo make install
sudo systemctl daemon-reload
sudo systemctl restart shairport-sync.service
```

If the unit file changed, verify it was updated as expected in the installed unit directory.
