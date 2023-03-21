SHELL := /usr/bin/env bash

# Skip ledclockgo for now, until I figure out how to set up a tinygo
# environment in GitHub Actions.
all:
	set -e; \
	for i in */Makefile; do \
		if [[ "$$i" =~ ledclockgo* ]]; then \
			echo '==== Skipping:' $$(dirname $$i); \
		else \
			echo '==== Making:' $$(dirname $$i); \
			$(MAKE) -C $$(dirname $$i); \
		fi \
	done

clean:
	set -e; \
	for i in */Makefile; do \
		echo '==== Cleaning:' $$(dirname $$i); \
		$(MAKE) -C $$(dirname $$i) clean; \
	done
