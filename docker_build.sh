docker build -t gbl_builder:latest .
docker image prune -f --filter label=org.opencontainers.image.title="gbl_builder"