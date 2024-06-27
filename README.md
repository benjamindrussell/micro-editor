# Micro Editor


https://github.com/benjamindrussell/micro-editor/assets/46113261/7bcb2669-36f0-4ade-b54a-f28353a0e0fb


I followed this tutorial: https://github.com/snaptoken/kilo-tutorial to make a text editor. 

I then went back through and refactored it to use the NCURSES library and added features such as vim editing modes. I will be adding more features soon.

## Navigation
<ul>
    <li>
        <ul>
            <b><li>h - left</li>
            <li>j - down</li>
            <li>k - up</li>
            <li>l - right</li></b>
        </ul>
    </li>
    <li>i - insert mode</li>
    <li>Arrow Keys</li>
    <li>Home / End</li>
    <li>PgUp / PgDn</li>
</ul>

## Commands

<ul>
    <span><b>While in normal mode</b></span>
    <li>:q - Quit</li>
    <li>:q! - force quit
    <li>:w - Write to disk</li>
    <li>:wq - write then quit</li>
    <li>:f - find word in file</li>
    <li>:e &ltfilename&gt - edit file (closes current file)</li>
</ul>

