// Copyright 2023 Rimas Misevičius
// Distributed under the BSD-style license that can be
// found in the LICENSE file.

(function () {
  // path segment to replace with version string
  const path_segment_ind = 2;
  // versions.txt is in the same folder as this script
  const versions_url = new URL("versions.txt", document.currentScript.src);

  window.addEventListener("DOMContentLoaded", event => {
    const list_ctl = document.getElementById("version-select");

    function on_change() {
      const path_segments = window.location.pathname.split('/');
      path_segments[path_segment_ind] = list_ctl.value;
      list_ctl.value = initial_dir; // back to this document version
      // ... and load document of other version
      window.location.pathname = path_segments.join('/');
    }

    // Initial version
    const initial_dir = window.location.pathname.split('/')[path_segment_ind];
    const initial_option = list_ctl.options[0];
    initial_option.selected = true;
    initial_option.value = initial_dir;
    // Change event
    list_ctl.addEventListener("change", on_change);

    // Fetch versions and populate versions list
    fetch(versions_url)
    .then(response => response.text())
    .then(text => {
      // Split text into lines and remove blank lines if present
      const versions = text
        .split(/\r?\n/gm)
        .map(line => line.trim())
        .filter(line => line.length > 0)
        .map(line => line.split(":"));
      // Populate versions list
      let append = false;
      versions.forEach(dir_ver => {
        const dir = dir_ver[0];
        const ver = dir_ver[dir_ver.length - 1];
        if (dir === initial_dir) {
          append = true;
        } else {
          const option = document.createElement('option');
          option.text = ver;
          option.value = dir;
          if (append)
            list_ctl.appendChild(option);
          else
            list_ctl.insertBefore(option, initial_option)
        }
      });
    });
  });
})();
