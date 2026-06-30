docker build -t gbl-build-env:latest .
docker image prune -f --filter label=org.opencontainers.image.title="gbl-build-env"