[33mcommit 6e5a178b3120dced7fa5c29c655cc22ea182df3d[m[33m ([m[1;36mHEAD -> [m[1;32mrelease/v5.3[m[33m, [m[1;31morigin/master[m[33m, [m[1;31morigin/HEAD[m[33m, [m[1;32mmaster[m[33m)[m
Merge: 1e0df2768c e8dc634108
Author: Mahavir Jain <mahavir@espressif.com>
Date:   Fri Oct 4 12:04:19 2024 +0800

    Merge branch 'bugfix/mbedtls_doc_update' into 'master'
    
    docs: update mbedtls guide for TLS 1.3 supported change
    
    See merge request espressif/esp-idf!33926

[33mcommit e8dc63410825543edae3b927f6fc2aa9835378b2[m
Author: Mahavir Jain <mahavir@espressif.com>
Date:   Mon Sep 30 10:44:44 2024 +0530

    docs: update mbedtls guide for TLS 1.3 supported change

[33mcommit 1e0df2768c1e6d93bcced658c350fe3f3828315f[m
Merge: c90dd52974 8c1ecc4d94
Author: Fu Hanxi <fuhanxi@espressif.com>
Date:   Thu Oct 3 16:16:54 2024 +0800

    Merge branch 'contrib/github_pr_14097' into 'master'
    
    build: Reduce verbosity of disabled component manager warning (GitHub PR)
    
    Closes IDFGH-13156
    
    See merge request espressif/esp-idf!33836

[33mcommit c90dd529745e31a6983ef8ecb38f252d18f6c1dd[m
Merge: ac6c54c997 c4acf3faad
Author: Mahavir Jain <mahavir@espressif.com>
Date:   Thu Oct 3 12:30:44 2024 +0800

    Merge branch 'feat/newlib-sbom-exclude_cve-2024-30949' into 'master'
    
    fix(newlib): sbom: add CVE-2024-30949 to cve-exclude-list
    
    See merge request espressif/esp-idf!33947

[33mcommit 8c1ecc4d948c70ad95df4225b057bfac6e118e01[m
Author: Nebojša Cvetković <nebkat@gmail.com>
Date:   Thu Jun 27 18:26:11 2024 +0100

    build: Reduce verbosity of disabled component manager warning
    
    closes https://github.com/espressif/esp-idf/pull/14097

[33mcommit c4acf3faad06db4f8c04ca16c7f0858ecc002b1d[m
Author: Alexey Lapshin <alexey.lapshin@espressif.com>
Date:   Fri Sep 6 18:27:09 2024 +0700

    fix(newlib): sbom: add CVE-2024-30949 to cve-exclude-list

[33mcommit ac6c54c997f68662ed2c0b6cb07edd3736175c16[m
Merge: 58051ca1d4 6a23790608
Author: Radek Tandler <radek.tandler@espressif.com>
Date:   Tue Oct 1 23:33:55 2024 +0800

    Merge branch 'feature/storage_remove_nvs_test_host' into 'master'
    
    remove(nvs_flash/test_nvs_host): Old make based host tests removed after complete migration
    
    See merge request espressif/esp-idf!27853

[33mcommit 58051ca1d4dedab45fc119c8bf6ae12d448562ba[m
Merge: 2f0dc01f9e 2c11c2ab81
Author: Aditya Patwardhan <aditya.patwardhan@espressif.com>
Date:   Tue Oct 1 22:54:41 2024 +0800

    Merge branch 'fix/secure_boot_documentation' into 'master'
    
    fix(docs): Fix secure boot v2 documentation
    
    Closes IDFGH-13716
    
    See merge request espressif/esp-idf!33594

[33mcommit 2f0dc01f9e292051e1f99c9218719ed1b20ceb4f[m
Merge: 1fa67f81f8 95f5b29ffc
Author: Mahavir Jain <mahavir@espressif.com>
Date:   Tue Oct 1 21:37:24 2024 +0800

    Merge branch 'feat/added_api_for_authmode_in_provisioning_demo' into 'master'
    
    feat(provisioning): Addded api to set Authmode in provisioning
    
    Closes IDF-10973
    
    See merge request espressif/esp-idf!33566

[33mcommit 1fa67f81f86e393c0f67409c7feb04c886c25df9[m
Merge: ffdf59a898 5bb10bc10d
Author: Kapil Gupta <kapil.gupta@espressif.com>
Date:   Tue Oct 1 21:34:18 2024 +0800

    Merge branch 'fix/wifi_netif_null_deref' into 'master'
    
    esp_wifi: Add null pointer checks to WiFi-netif APIs
    
    Closes IDFGH-7092
    
    See merge request espressif/esp-idf!33946

[33mcommit 6a23790608804b155b3f556dd4e58bffc4d8ae2d[m
Author: radek.tandler <radek.tandler@espressif.com>
Date:   Mon Dec 11 18:24:43 2023 +0100

    remove(nvs_flash/test_nvs_host): Old make based host tests removed after complete migration

[33mcommit 95f5b29ffcbec8934607cc2dffa50d7229d70d53[m
Author: hrushikesh.bhosale <hrushikesh.bhosale@espressif.com>
Date:   Tue Sep 17 14:09:39 2024 +0530

    feat(provisioning): Addded api to set Authmode in provisioning
    
    Added api to set threshold authmode for provisioning demo

[33mcommit ffdf59a89851d81e8ec898ba7dc6a5cd7f1cac5b[m
Merge: 1143b7639f 8e532baf1e
Author: Erhan Kurubas <erhan.kurubas@espressif.com>
Date:   Tue Oct 1 18:03:39 2024 +0800

    Merge branch 'jtag_doc_update' into 'master'
    
    docs(jtag): update OpenOCD related sections
    
    See merge request espressif/esp-idf!33516

[33mcommit 5bb10bc10d73f42b684c4dcdc46fc9e96a0a363c[m
Author: David Cermak <cermak@espressif.com>
Date:   Tue Oct 1 11:28:10 2024 +0200

    fix(esp_wifi): Add null pointer checks to WiFi-netif APIs
    
    Added null pointer checks to WiFi-netif API functions to prevent potential
    crashes from invalid arguments, updated unit tests.
    
    Closes https://github.com/espressif/esp-idf/issues/8702

[33mcommit 1143b7639f790b5550e342660037b3d061f9986d[m
Merge: 4b3a5e93cf d77d1f5a95
Author: Roland Dobai <roland@espressif.com>
Date:   Tue Oct 1 17:12:29 2024 +0800

    Merge branch 'fix/idfpy_uf2_fail' into 'master'
    
    fix(uf2): Ignore ESPBAUD, ESPPORT when calling idf.py uf2[-app]
    
    Closes IDF-10768
    
    See merge request espressif/esp-idf!33086

[33mcommit 4b3a5e93cfe33addb5854c4903af12ab4be7d537[m
Merge: 0db772efff 6ff7947fef
Author: Fu Hanxi <fuhanxi@espressif.com>
Date:   Tue Oct 1 16:49:39 2024 +0800

    Merge branch 'feat/core_components' into 'master'
    
    feat: add `COMPONENT_SOURCE` property to component targets
    
    Closes DOC-7429
    
    See merge request espressif/esp-idf!26586

[33mcommit 0db772efff92eba379b2b2deaa40802afdcd74b2[m
Merge: d50a64ea84 0d4787fb3a
Author: Konstantin Kondrashov <konstantin@espressif.com>
Date:   Tue Oct 1 16:25:00 2024 +0800

    Merge branch 'feature/new_esp_partition_api' into 'master'
    
    feat(esp_partition): Adds new esp_partition APIs
    
    See merge request espressif/esp-idf!33497

[33mcommit 2c11c2ab8135c3c7064fb5c262fc17a457cf4202[m
Author: Aditya Patwardhan <aditya.patwardhan@espressif.com>
Date:   Tue Sep 17 22:07:05 2024 +0530

    fix(docs): Fix secure boot v2 documentation
    
        Update the secure boot v2 documentation
        about the limitation of read-protection
        when secure boot is enabled
    
        Closes https://github.com/espressif/esp-idf/issues/14587

[33mcommit d50a64ea84e124943eaa7684e30a263b7763517c[m
Merge: 97b114643f 70f222e5d2
Author: Roman Leonov <roman.leonov@espressif.com>
Date:   Tue Oct 1 01:33:05 2024 +0800

    Merge branch 'feature/usb_host_ext_port_driver' into 'master'
    
    feat(ext_port): Added External Port Driver
    
    Closes IDF-9698, IDF-10237, IDF-10054, IDF-10070, and IDFGH-11416
    
    See merge request espressif/esp-idf!30392

[33mcommit 97b114643f4e27f2331e7f7bbb4f21e537feb1c5[m
Merge: 16d99f3e10 503097739e
Author: Kevin (Lao Kaiyao) <laokaiyao@espressif.com>
Date:   Mon Sep 30 23:04:29 2024 +0800

    Merge branch 'bugfix/workaround_of_i2s_half_sample_rate_issue' into 'master'
    
    fix(i2s): fix i2s half sample rate issue
    
    Closes IDF-11278
    
    See merge request espressif/esp-idf!33837

[33mcommit 503097739e9381f6e1c66164ffc324ab641b987b[m
Author: laokaiyao <laokaiyao@espressif.com>
Date:   Thu Sep 26 17:49:31 2024 +0800

    fix(i2s): fix multi-dev test failure

[33mcommit 5be7860c540ebe161db409ced30d75f713360b7a[m
Author: laokaiyao <laokaiyao@espressif.com>
Date:   Thu Sep 26 15:38:25 2024 +0800

    fix(i2s): fix i2s half sample rate issue

[33mcommit 16d99f3e1081b153e0d5bc015f4178847a973cf0[m
Merge: ad6fba3cc8 4ed1b873b4
Author: Kevin (Lao Kaiyao) <laokaiyao@espressif.com>
Date:   Mon Sep 30 22:16:05 2024 +0800

    Merge branch 'feature/support_i2s_retention' into 'master'
    
    feat(i2s): support sleep retention
    
    Closes IDF-8468, IDF-9754, IDF-9783, and IDF-10343
    
    See merge request espressif/esp-idf!33529

[33mcommit 6ff7947fefc637c5e714b9b5c326179adfcca9c7[m
Author: Fu Hanxi <fuhanxi@espressif.com>
Date:   Thu Oct 19 17:21:27 2023 +0200

    feat: add component_source property to component targets

[33mcommit ad6fba3cc8bf2d8c0f2cde9cd77304bc1d130a85[m
Merge: b835986b62 d618d1d934
Author: Jiang Jiang Jian <jack@espressif.com>
Date:   Mon Sep 30 17:41:12 2024 +0800

    Merge branch 'fix/fix_free_eb_twice_issue' into 'master'
    
    fix(wifi): fix ampdu to normal cause free buffer twice issue
    
    Closes WIFIBUG-776
    
    See merge request espressif/esp-idf!33894

[33mcommit 4ed1b873b42056b64cbba7c61fa7f74ab37c758e[m
Author: laokaiyao <laokaiyao@espressif.com>
Date:   Wed Sep 25 16:35:41 2024 +0800

    fix(gdma): fixed GDMA retention on C5

[33mcommit ae36f8494520ae2682aa486e2016de8f3fbc363d[m
Author: laokaiyao <laokaiyao@espressif.com>
Date:   Sat Sep 14 18:27:27 2024 +0800

    feat(i2s): enable i2s test on c5

[33mcommit 0cb4bdc54e045328851857aa24b2fbb438407e61[m
Author: laokaiyao <laokaiyao@espressif.com>
Date:   Fri Sep 13 21:56:02 2024 +0800

    feat(i2s): support sleep retention

[33mcommit b835986b62ffa02102bfa0e6e8c3bdd36ceacbd6[m
Merge: 76b3cf5178 1c75f46fb9
Author: Zhang Wen Xu <zhangwenxu@espressif.com>
Date:   Mon Sep 30 17:31:45 2024 +0800

    Merge branch 'feat/support_vendor_hook_for_rcp_spi' into 'master'
    
    feat(openthread): support vendor hook for rcp spi
    
    Closes IDFGH-13771
    
    See merge request espressif/esp-idf!33773

[33mcommit 76b3cf517811532dd0ae53b9f7bd91cc63c72ed8[m
Merge: a6091dfbba 1b770c3931
Author: Mahavir Jain <mahavir@espressif.com>
Date:   Mon Sep 30 17:03:56 2024 +0800

    Merge branch 'fix/mbedtls_fs_io_dependency_on_vfs' into 'master'
    
    fix(mbedtls): Fix the MBEDTLS_FS_IO dependency on vfs
    
    Closes IDFGH-13518
    
    See merge request espressif/esp-idf!33020

[33mcommit a6091dfbba806c078dfa35d74d737630e3e65cd3[m
Merge: ffa74e3b46 397fdb56fb
Author: Fu Hanxi <fuhanxi@espressif.com>
Date:   Mon Sep 30 16:32:27 2024 +0800

    Merge branch 'ci/upload-build-size-artifacts' into 'master'
    
    ci: ensure binary size artifacts are uploaded in Build Stage Child pipeline via artifacts_handler script.
    
    See merge request espressif/esp-idf!33577

[33mcommit 1b770c39312c95814c4f8107aefe2dbe7e2ff486[m
Author: Aditya Patwardhan <aditya.patwardhan@espressif.com>
Date:   Thu Aug 22 17:43:17 2024 +0530

    fix(mbedtls): Fix the MBEDTLS_FS_IO dependency on vfs
    
    By default MBEDTLS_FS_IO option in mbedtls uses the filesystem supported
    added by vfs component.
    If the vfs support is disabled by user then mbedtls raises a warning
    that the filesystem realted operation shall always fail
    This commit fixes the behaviour by enabling respective depedency check for the
    MBEDTLS_FS_IO option
    
    Closes https://github.com/espressif/esp-idf/issues/14409

[33mcommit ffa74e3b464efe9533fead23df94a38aca6025e9[m
Merge: c5fc2053c9 6e4d3104b2
Author: Jiang Jiang Jian <jack@espressif.com>
Date:   Mon Sep 30 16:09:06 2024 +0800

    Merge branch 'bugfix/fix_some_macro_definition_not_correct' into 'master'
    
    fix(wifi): fix some macro definition not correct
    
    See merge request espressif/esp-idf!33927

[33mcommit c5fc2053c9669056aa0137388b58ed747084e62c[m
Merge: 07400d348c 13c8c2f121
Author: Alexey Gerenkov <alexey@espressif.com>
Date:   Mon Sep 30 16:01:26 2024 +0800

    Merge branch 'fix/coredump_sha_build_error' into 'master'
    
    fix(esp-coredump): fix build error when mbedtls hardware sha disabled
    
    See merge request espressif/esp-idf!33841

[33mcommit 6e4d3104b2166421b73534b5f5f96a20245171da[m
Author: yinqingzhao <yinqingzhao@espressif.com>
Date:   Mon Sep 30 15:34:37 2024 +0800

    fix(wifi): fix some macro definition not correct

[33mcommit 07400d348c8781b1f119e989a758551d6e94b2c7[m
Merge: af017d143a 30c13225ca
Author: David Čermák <cermak@espressif.com>
Date:   Mon Sep 30 15:37:40 2024 +0800

    Merge branch 'feat/wifi_remote_with_hosted' into 'master'
    
    fix(examples): esp_hosted as default for Wi-Fi connect on P4
    
    See merge request espressif/esp-idf!33213

[33mcommit af017d143a24f8d9e75c053ac6fd91c138905df8[m
Merge: ed5f75abe6 5af61242d2
Author: David Čermák <cermak@espressif.com>
Date:   Mon Sep 30 15:15:29 2024 +0800

    Merge branch 'feat/docs_wifi_on_wifiless_chips' into 'master'
    
    feat(wifi): Document WiFi expansion on wifi-less chips
    
    See merge request espressif/esp-idf!31418

[33mcommit 8e532baf1e900b45c37fd909c3c89d3dc829f9a8[m
Author: Erhan Kurubas <erhan.kurubas@espressif.com>
Date:   Fri Sep 13 11:37:37 2024 +0300

    docs(jtag): update OpenOCD related sections

[33mcommit 5af61242d24d796013109a0f6f4b06e6f6e1a883[m
Author: David Cermak <cermak@espressif.com>
Date:   Mon Jun 10 10:06:22 2024 +0200

    feat(wifi): Document WiFi expansion on wifi-less chips

[33mcommit ed5f75abe6ac2b11052d3495a6c906e5e08304f5[m
Merge: 945ab1a8fa d918aa3ebd
Author: Jiang Jiang Jian <jack@espressif.com>
Date:   Mon Sep 30 13:38:02 2024 +0800

    Merge branch 'bugfix/fix_miss_internal_wapi_deinit_issue' into 'master'
    
    fix(wifi): fix miss internal wapi deinit issue
    
    Closes WIFI-6355
    
    See merge request espressif/esp-idf!33920

[33mcommit d618d1d934e1d54e57adfce0320abcf0e10d6392[m
Author: wangtao@espressif.com <wangtao@espressif.com>
Date:   Fri Sep 27 18:39:57 2024 +0800

    fix(wifi): fix ampdu to normal cause free buffer twice issue

[33mcommit 945ab1a8fa15613288b703df0bd964c3f82b3b37[m
Merge: 1522b6bb8e 3169af7341
Author: Zhang Xiao Yan <zhangxiaoyan@espressif.com>
Date:   Mon Sep 30 11:23:53 2024 +0800

    Merge branch 'docs/update_front_page_c5' into 'master'
    
    docs: update front page for esp32-c5 and esp32-c61
    
    See merge request espressif/esp-idf!33907

[33mcommit 3169af73412891ceeb5c6346d016ffdda7b0cb92[m
Author: Linda <zhangxiaoyan@espressif.com>
Date:   Sun Sep 29 14:56:10 2024 +0800

    docs: update front page for esp32-c5 and esp32-c61

[33mcommit d918aa3ebd7d18b79d4b7f9a9cfe94ae20b764a3[m
Author: muhaidong <muhaidong@espressif.com>
Date:   Wed Nov 22 11:38:37 2023 +0800

    fix(wifi): fix miss internal wapi deinit issue

[33mcommit 1522b6bb8e50255d9b148d6e291e33d75ffed619[m
Merge: fcdbff5039 9ec9071876
Author: Jiang Jiang Jian <jack@espressif.com>
Date:   Sun Sep 29 22:59:19 2024 +0800

    Merge branch 'bugfix/fix_retry_assoc_auth_timeout_issue' into 'master'
    
    fix(wifi): fixed association refused temporarily lead auth timeout issue
    
    Closes WIFI-6699
    
    See merge request espressif/esp-idf!33780

[33mcommit fcdbff5039308f74fd9f4a14ce7466628d95e15e[m
Merge: 906f36bdb9 0216236d0c
Author: Nilesh Kale <nilesh.kale@espressif.com>
Date:   Sun Sep 29 22:33:45 2024 +0800

    Merge branch 'feature/update_mbedtls_version_to_3.6.1' into 'master'
    
    feat(mbedtls): updated mbedtls version to 3.6.1
    
    Closes IDF-10980
    
    See merge request espressif/esp-idf!33282

[33mcommit 906f36bdb9bcc572fff5ac26a305e2d166895f7c[m[33m ([m[1;31morigin/release/v5.4[m[33m)[m
Merge: 42f1e2177a f85ef40dcd
Author: Jiang Jiang Jian <jack@espressif.com>
Date:   Sun Sep 29 20:04:21 2024 +0800

    Merge branch 'bugfix/idf-10972' into 'master'
    
    fix the coverity issue of 'Constant' variable guards dead code
    
    Closes IDF-10972
    
    See merge request espressif/esp-idf!33788

[33mcommit 42f1e2177ad7ab0c799a2abcac56a2ceaba39fdf[m
Merge: 8ed0508320 beda284524
Author: Jiang Jiang Jian <jack@espressif.com>
Date:   Sun Sep 29 19:47:26 2024 +0800

    Merge branch 'feature/add_dpp_crypto_layer' into 'master'
    
    feature(esp_wifi):Restructure dpp crypto layer
    
    See merge request espressif/esp-idf!30225

[33mcommit 8ed0508320f30516bdfe440bd693c097df00c0f3[m
Merge: 12cbdfef31 3da968e122
Author: Jiang Jiang Jian <jack@espressif.com>
Date:   Sun Sep 29 19:27:31 2024 +0800

    Merge branch 'change/polish_wifi_sleep_current_desc' into 'master'
    
    change(doc): add notes for sleep current data tested in shieled box
    
    Closes WIFI-6711
    
    See merge request espressif/esp-idf!33796

[33mcommit beda28452437578de2e7a4b7ebbf3e9c75fa89dc[m
Author: aditi <aditi.lonkar@espressif.com>
Date:   Fri Jul 26 16:41:56 2024 +0530

    feat(esp_wifi): Add esp-idf specific changes
    
        Added esp-idf implementation specific changes on top of the upstream updates.

[33mcommit fbde07c95377b39d3fad1b4029405ae402f467a1[m
Author: aditi <aditi.lonkar@espressif.com>
Date:   Tue Jul 23 18:27:43 2024 +0530

    feat(esp_wifi): Replace crypto_key with crypto_ec_key
    
        Replaced all occurances of crypto_key with crypto_ec_key struct
        to make the code more consistent with upstream.
