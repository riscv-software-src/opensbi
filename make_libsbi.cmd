set p=%cd%

docker run -v %p%:/app -it --rm pafos/releng:main /app/cross_build.sh
