# Hell's Kitchen

### How do I get set up? ###

Create a file "settings.cmake" in the project folder.
Windows users need to specify path to visual studio include and library files.
```
#!cmake

set(CUSTOM_INCLUDE "C:/Program Files (x86)/Windows Kits/10/Include/10.0.15063.0/um")
set(CUSTOM_LIBRARY "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.15063.0/um/x86")

```

This is also not an electric cat.

    oooooooooooooooooooooooooooooosoosssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss
    ooooooooooooooooooooooooooooooooooooooooossossssssssssssssssssssssosssssssssssssssssssssssssssssssss
    ooooooooooooooooooooooooooooooooossooooooooooooooossosssssssssosssoossssssssssssssssssssssssssssssss
    ooooooooooooooooooooosyyyyyysossyyosssossoooooooooooooooooooooooossssossoossssoooooooooosoosoosooooo
    oooooooooooooooooooshdhyysyysoosyyssyyyyhhyhysoooooooooooooooooooooooooooooooooooooooooooooooooooooo
    ooooooooooooooooooshmmmmddhs++syhsosshhhhmddmhyyoooooooooooooooooooooooooooooooooooooooooooooooooooo
    oooooooooooooooooyhhyhyyysooossyyyoosydmmmmmmmmmsooooooooooooooooooooooooooooooooooooooooooooooooooo
    oooooooooooooooosyhhysso+/+syhhhhhhdddhhyyshdmNNhsoooooooooooooooooooooooooooooooooooooooooooooooooo
    +++ooooooooooooosdddyyysoohdmddmddmmdhhhhyyyhdmmmsoooooooooooooooooooooooooooooooooooooooooooooooooo
    ++++ooooooooooooydhhhhhyyyyo+++ooshhhhhhddhyhhhmhooooooooooooooooooooooooooooooooooooooooooooooooooo
    ++++++ooooooooooyyhddhyss+//::::-:::://+oshdddyhhooooooooooooooooooooooooooooooooooooooooooooooooooo
    +++++++o+oooooooohshhsso+///:::---::::////+sshyhdooooooooooooooooooooooooooooooooooooooooooooooooooo
    +++++++++++oooooohhhhss+////::::::::::////++shhdyooooooooooooooooooooooooooooooooooooooooooooooooooo
    ++++++++++++++oooydmmyo+////::::::::::////++ymddoooooooooooooooooooooooooooooooooooooooooooooooooooo
    +++++++++++++++++shmmyo++osys+/::::/+so+++++ymdy+ooooooooooooooooooooooooooooooooooooooooooooooooooo
    ++++++++++++++++++oyds+ooshhsss/:/ossyhysoooyds+oooooooooooooooooooooooooooooooooooooooooooooooooooo
    ++++++++++++++++++/+ho///++/////://///+++//+ys//oooooooooooooooooooosmhooooooooooooooooooooooooooooo
    ++++++++++++++++++::yy+/:::::/+/:///::::::/+y//+ooooooooooooooooooosNMMyoooooooooooooooooooooooooooo
    ++++++++++++++++++//yh+//::::/+/:/++::::://sy:/ooooooooooooooooooosNMMMmooooooooooooooooooooooooooo+
    ++++++++++++++++++++oho+/::::+s+/+o+:::://+ys+oooooooooooooooooooomMMMMmoooooooooooooooooooooooo+o++
    +++++++++++++++++++++hy+//:://osso++::///+syooooooooooooooooooooodMMMMMdoooooooooooooooo++ooo+++++++
    +++++++++++++++++++++hho+///++++/++++////oysooooooooooooooooooooyMMMMMMyooooooooooooo+++++++++++++++
    +++++++++++++++++++++sds+//////++++++///+oo+oooooooooo+oooooooooNMMMMMNoooooooooo+oo++++++++++++++++
    +++++++++++++++++++:-:oho+/////////////+os+++++oo++ooooo+oooooodMMMMMMyoooooooohmoo+++++++++++++++++
    +++++++++++++++++/:.`..-ooo+/////////+oydy+++++++++++oo+ooooooyMMMMMMmoo++++oyNMMs++++++++++++++++++
    +++++++++++++++/-.``````.-:/+++++/++ohdhho/-.--:://+++++ooo+ooNMMMMMMs+++++smMMMMs++++++++++++++++++
    +++++++++++++/-.``````````..-::/oydhyyyso+-.....--..-:/++o++omMMMMMMd++++ohMMMMMm+++++++++++++++++++
    +++++++++++/-```````````````......-syso+/-`.............-/++yMMMMMMNo+++sNMMMMMMs+++++++oyhyo+++++++
    +++++++++/:...`````````````````````.--..```...`............:dMMMMMMy++ohMMMMMMNy++++++oshmydmho+++++
    +++++++//-.......`````````````````````.`````.`.`......`....ysdNmmNh++smMMMMMMNs++++++shmNNNMMMNho+++
    +++++//:-...````....``````````````````````.``.`..``....`.`+NNN+//oo+sNMMMMMMmo+++++ohmNNMMMMMMMMNs++
    ++++/:-..``````````...````````````````````.`````````...`.-mMMy...:+dhmMMMMMdo++++oymNNMMMMMMMMMNho++
    +++/:..````````.`````...``````````````````.```````````.`-hNMN-`../mmNmsshdy+++++sdNMMMMMMMMMMNho++++
    ++:-....```````````````...``````````````````````.``.``-:+NMms/:-sNMNy::/+o+++oshNNMMMMMMMMMNdo++++++
    +/-.....`````````````````....```````````````````...`.--:ymd++oodMmNo....:/oymNNNMMMMMMMMMNdo++++++++
    +/-....`````````````````````..``````````.......--:.`---:yo/::+ohhys+/...:smMMNhsymMMMMMNho++++++++++
    +/-....``````````````````````````````...`...````````-:://////+///+sso./hNMMNhs+++ohNMmho++++++++++++
    //-.....``````````````````````.-.```````````````````-:::://///////+osdNMMmhsosso+++oso++++++++++++++
    -:--....``````````````````````-:::-..```````````````-///::////////++ymmmhssssssssoo+++++++++++++++++
    ..---..```````````````````````.-:::::-------.....``.+sso+//+//++++oo+osooooooooooosoo+++++++++++++++
    .-.-.....```````````````.```````.-:////:::://///:::/odNmds+++++++++++++++++++++oooosso++++++++++++++
    -.-`.......```````````````````````.-//::::::://:::::/dddmhh+/////////////++++++ooooosss+++++++++++++
    :-:.`....``````````````````````.-::/::::::::::::::--/oyddmm++///://///////++++++ooooooos++++++++++++
    /--:..``....`````````````````.-:::///:::::::::::------+mNNh///////////++++++++oooooooooso+++++++++++
    +/:::...`````````````````````.-://+///::::::::::::::::+NNm+///////////+++++++++oooooossso+++++++++++
    +++//:....````````````````````.-++///+//::::::::::/+osmNds++/////////++++++++ooossssysss++++++++++++
    ++++/+:......``````````````````.-/+o+////::///://oyhyyysshhysoooooooooooooosssyyyyyysso+++++++++++++
    ++++//+-..`````````````````````..+//++o+++///+osss+//++/+osho+++ooossssyyyyyyyyyso++++++++++++++++++
    +++++///-..``````````````````````:+so+++++o+++//////++ooo++so+++++++++++++++++++++++++++++++++++++++
    +++++////...````````````````````./o+++++++////////++ooo+:-/oo+++++++++++++++++++++++++++++++++++++++
    ++++++:///...```````````````.-:/oo+++++////////+//++ss+-..//++++++++++++++++++++++++++++++++++++++++
