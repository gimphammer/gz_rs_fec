# gz_rs_fec
This is an implementation of Reed–Solomon FEC.



Performance data: 

On M4 chipset, when (n,k) = (96, 32), package data lenght = 1200 bytes, the dec/enc timecost is about 1ms by release version. In this case, FEC can resist 66% package lost.



Platform Support:

Now is only support Mac. But it's easy to port to other platforms basd on C++.



Some work in the future:

Now, the gz_rs_fec is based on C++ STL. The performance is enough for 1080p video FEC.

But we can still reduce some time cost by C version, or assembly instructions.



