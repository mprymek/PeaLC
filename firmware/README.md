## Requisites

- PlatformIO

## Sources Preparation

At first, you must fetch, patch and build thirdparty software. You can do it
with:
```sh
$ make -C thirdparty
```

## Quickstart

```sh
$ . ~/.platformio/penv/bin/activate
(penv) $ make build plc_program=st/blink.st
```

## Legal

Firmware uses software from various thirdparty sources described below.

[OpenPLC Runtime][openplc] sources are fetched from [it's Github repository][openplc-github]
into `thirdparty/OpenPLC_v3` directory. According to the project author,
[it's licensed under GPL v.3][openplc-license] but there's
[ongoing change to Apache 2 license][openplc-license2]. If you have any doubt,
please contact project owners directly.

The fetched OpenPLC sources also contain *Matiec compiler* sources in the
`thirdparty/OpenPLC_v3/utils/matiec_src` directory. We use this compiler to compile
Structured Text programs into C sources but do not link any part of it into the firmware.
Matiec [is licensed under GPL v.3][matiec-license].

[Libcanard][libcanard] at `thirdparty/libcanard` [is licensed under
MIT License][libcanard-license].

[libcanard]: https://github.com/UAVCAN/libcanard
[libcanard-license]: https://github.com/UAVCAN/libcanard/blob/master/LICENSE
[matiec]: https://bitbucket.org/mjsousa/matiec
[matiec-license]: https://bitbucket.org/mjsousa/matiec/src/default/COPYING
[openplc]: https://www.openplcproject.com/
[openplc-github]: https://github.com/thiagoralves/OpenPLC_v3
[openplc-license]: https://openplc.discussion.community/post/license-of-openplc-runtime-for-linux-10059730
[openplc-license2]: https://github.com/thiagoralves/OpenPLC_v3/issues/17
