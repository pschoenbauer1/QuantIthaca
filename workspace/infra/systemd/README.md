# Systemd User Services

## Services

| File | Description |
|------|-------------|
| `quantithaca.service` | QuantIthaca Streamlit app (port 10000) |
| `rclone-onedrive.service` | Rclone OneDrive mount |

## Install

```bash
bash infra/systemd/install.sh
```

Copies all service files to `~/.config/systemd/user/`, enables and starts them, and runs `loginctl enable-linger` so they survive logout.

## Logs

Streamlit output is appended to:

```
~/.local/share/quantithaca/streamlit.log
```

Follow live:

```bash
tail -f ~/.local/share/quantithaca/streamlit.log
```

## Common commands

```bash
# View all installed user services and their state
systemctl --user list-unit-files --type=service

# View running user services
systemctl --user list-units --type=service

# View all (including inactive)
systemctl --user list-units --type=service --all

# Status / logs
systemctl --user status <service>
journalctl --user -u <service> -f

# Start / stop / restart
systemctl --user start <service>
systemctl --user stop <service>
systemctl --user restart <service>
```
