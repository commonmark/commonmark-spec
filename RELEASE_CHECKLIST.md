Release checklist

_ update changelog.txt
_ update version in package.json
_ tag release
_ git push
_ git push --tags
_ github release
_ npm publish
_ update dingus:

    cd ../commonmark.js
    cd dingus
    make

_ update site:

    cd ../../CommonMark-site
    make update
    make
    make upload
