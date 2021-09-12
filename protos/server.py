#!/usr/bin/env python3

# Copyright 2015 gRPC authors.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
"""The Python implementation of the merklefs object.Fetcher server."""

from concurrent import futures
import json
import logging
import subprocess

import grpc
import object_pb2
import object_pb2_grpc

class Fetcher(object_pb2_grpc.FetcherServicer):

    def __init__(self, remote, pool):
        self._remote = remote
        self._pool = pool

    def Fetch(self, request, context):
        print('fetch', request.key)
        url = f'{self._remote}/{request.key}'
        local = f'{self._pool}/{request.key}'
        p = subprocess.run(['wget', url, '-O', local])
        success = p.returncode == 0
        return object_pb2.FetchReply(ok=success)


def serve():
    with open('/etc/merklefs/config.json', 'r') as f:
        j = json.load(f)
    server = grpc.server(futures.ThreadPoolExecutor(max_workers=10))
    fetcher = Fetcher(remote=j['remote'], pool=j['pool'])
    object_pb2_grpc.add_FetcherServicer_to_server(fetcher, server)
    #server.add_insecure_port('[::]:50051')
    server.add_insecure_port(j['fetcher'])
    server.start()
    server.wait_for_termination()


if __name__ == '__main__':
    logging.basicConfig()
    serve()
