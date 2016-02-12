#TTBaseHost

64ビットの TTBase 互換アプリケーションで 32ビットのプラグインを動作させようとする試みです。  
**Visual Studio Community 2015 (C++11, 32/64-bit)** でコンパイルを確認しています。

##内容

---

- TTBaseBridge.sln
- BridgeData.hpp
- x86.manifest
- amd64.manifest
- NYSL
- [TTBbridge]
 - TTBbridge.vcxproj
 - TTBbridge.vcxproj.filters
 - TTBbridge.vcxproj.users
 - WinMain.cpp
 - TTBasePluginAdapter.hpp
 - Utility.hpp
 - [include]
  - File.hpp
  - GenerateUUIDString.hpp
  - Transcode.hpp
 - [TTBase]
  - Plugin.hpp
- [TTBMiniHost]
 - TTBMiniHost.vcxproj
 - TTBMiniHost.vcxproj.filters
 - TTBMiniHost.vcxproj.users
 - WinMain.cpp
 - TestWnd.hpp
 - TTBBridgePlugin.hpp
 - Utility.hpp
 - [include]
  - Application.hpp
  - CtrlWnd.hpp
  - File.hpp
  - Font.hpp
  - GenerateUUIDString.hpp
  - UWnd.hpp
  - UWnd.cpp
 - [TTBase]
  - Plugin.hpp

---

##説明
　プロセス間通信の技術を使って、64ビットのホストから32ビットのプラグインを読み込み、実行します。  

###TTBbridge
　ビット数の境界を超えるためのブリッジアプリケーションです。  
　プラグイン一つにつき、一つのインスタンスが立ち上がります。また、通信用の共有メモリとして、インスタンス一つにつき 少なくとも 160 バイトを消費します。  

##TTBMiniHost
　動作確認のためのミニプログラムです。プラグインの読み込み、解放、コマンドの実行が行えます。  
　同時に読み込めるプラグインは一つだけです。

##ライセンス

NYSL Version 0.9982
```
A. 本ソフトウェアは Everyone'sWare です。このソフトを手にした一人一人が、
   ご自分の作ったものを扱うのと同じように、自由に利用することが出来ます。

  A-1. フリーウェアです。作者からは使用料等を要求しません。
  A-2. 有料無料や媒体の如何を問わず、自由に転載・再配布できます。
  A-3. いかなる種類の 改変・他プログラムでの利用 を行っても構いません。
  A-4. 変更したものや部分的に使用したものは、あなたのものになります。
       公開する場合は、あなたの名前の下で行って下さい。

B. このソフトを利用することによって生じた損害等について、作者は
   責任を負わないものとします。各自の責任においてご利用下さい。

C. 著作者人格権は tapetums に帰属します。著作権は放棄します。

D. 以上の３項は、ソース・実行バイナリの双方に適用されます。
```

NYSL Version 0.9982 (en) (Unofficial)
```
A. This software is "Everyone'sWare". It means:
  Anybody who has this software can use it as if he/she is
  the author.

  A-1. Freeware. No fee is required.
  A-2. You can freely redistribute this software.
  A-3. You can freely modify this software. And the source
      may be used in any software with no limitation.
  A-4. When you release a modified version to public, you
      must publish it with your name.

B. The author is not responsible for any kind of damages or loss
  while using or misusing this software, which is distributed
  "AS IS". No warranty of any kind is expressed or implied.
  You use AT YOUR OWN RISK.

C. Copyrighted to tapetums

D. Above three clauses are applied both to source and binary
  form of this software.
```

[http://www.kmonos.net/nysl/](http://www.kmonos.net/nysl/)

---

##謝辞

TTBase を生み出された K2 さん、peach を 公開されている U さん、TTBaseCpp の作者さんを始め、  
数多くのプラグインを作られたそれぞれの作者さんたちに深い敬意と感謝を表します。

---

###変更履歴

2016.02.12  v0.1.0.0
- 初版発行

---

以上です。

####文責
tapetums

######この文書のライセンスは NYSL Version 0.9982 とします。  
######[http://www.kmonos.net/nysl/](http://www.kmonos.net/nysl/)
