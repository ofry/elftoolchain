inittest movebefore_movepos-liba-v tc/movebefore_movepos-liba-v
extshar ${TESTDIR}
extshar ${RLTDIR}
runcmd "../ar mbv a2.o liba.a a4.o a2.o a1.o" work true
runcmd "plugin/teraser -ce -t movebefore_movepos-liba-v liba.a" work false
runcmd "plugin/teraser -e liba.a" result false
rundiff true
