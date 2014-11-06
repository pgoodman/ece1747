Moldyn with Intel TSX.
=============================

Mike Dai Wang, dai.wang AT mail.utoronto.ca

## Requirements:
- Intel Haswell CPU with TSX support 
- gcc 4.8 or another compiler with TSX support

To Make:
``./make``

To use:
link your application with lib_tm.a through ``-l_tm`` flag

Detailed Description:
  Based on the original LibTM design, the new implementation incorporates the
lastest Intel TSX extensions with Hardware TM support. It is intended to be
used in performance and optimization studies.

## Included Applications
### Moldyn

Moldyn is a 3D molecular dynamics simulation tool.

To Make:
``./make moldyn``

To Clean:
``./make moldyn_clean``

To Run:
``./moldyn #_of_threads #_of_max_txn_retries``

