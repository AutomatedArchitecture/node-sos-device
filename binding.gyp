{
  "targets": [
    {
      "target_name": "sos",
      "sources": [
        "src/binding.cpp",
        "src/nodeSos.cpp"
      ],
      "cflags": ['-fexceptions'],
      "cflags_cc": ['-fexceptions'],
      "conditions" : [
        [
          'OS!="win"', {
            "libraries" : [
              '-lusb'
            ]
          }
        ],
        [
          'OS=="win"', {
            "libraries" : [
              '<(module_root_dir)/gyp/lib/libusb.dll.a'
            ]
          }
        ]
      ]
    }
  ]
}
