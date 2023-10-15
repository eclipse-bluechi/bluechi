# Go variables
GO := go
GOFMT := gofmt
GOBUILD := $(GO) build
GOCLEAN := $(GO) clean

# Main package name
MAIN_PACKAGE := bluechi-tester.go

BINARY_NAME := bluechi-tester

# Targets
all: fmt build

build:
	$(GOBUILD) -o $(BINARY_NAME) $(MAIN_PACKAGE)

clean:
	$(GOCLEAN)
	rm -f $(BINARY_NAME)

fmt:
	$(GOFMT) -w .

.PHONY: all build clean fmt

