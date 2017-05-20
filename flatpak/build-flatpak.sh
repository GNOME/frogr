#!/bin/bash

flatpak-builder --force-clean --ccache --require-changes --repo=repo app $1
