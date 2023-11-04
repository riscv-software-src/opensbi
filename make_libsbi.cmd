set p=%cd%

REM docker run --user %user% -v %p%:/app -it --rm pafos/releng:main /app/cross_build.sh
docker run -v %p%:/app -it --rm pafos/releng:main /app/cross_build.sh
