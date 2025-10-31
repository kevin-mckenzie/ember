
### venv management
```
# needed
sudo apt install python3-venv

# create
python3 -m venv ./.venv

#activate
source ./.venv/bin/activate

# Now we are in the venv
python3 -m ensurepip --upgrade
python3 -m pip install -r requirements.txt

# When we want to compile requirements.in into requirements.txt (or pyproject.toml? see about this at packaging time)
python3 -m install pip-tools
pip-compile requirements.in

# Creates requirements.txt
python3 -m pip install -r requirements.txt
```


TODO:
 - figure out if misc include cleaner is really a good idea... false positives on NULL

Goals for this project:
 - learn asyncio fluency in python
 - get better at python in general, it's been a while
 - execute with mmap() instead of shm/memfd_create
 - learn basic TUI (or web) dev
 - demonstrate work experience in minimal implementation, something to talk about in interviews
    - size, stamping, jitter, stack-focus, security, consistent style
