Design Doc: https://app.eraser.io/workspace/51LJBr86c4kcQaaVYSAq?origin=share

**TODOs**:
- [x] Implement repo initialization
- [x] Hash blob to store in .git/objects/
- [x] Hash tree to store in .git/objects/
- [x] Hash commit to store in .git/objects
- [ ] Implment staging area (index)
- [ ] Implement get previous hash for a file from previous commit
- [ ] Implement branches (refs)
- [ ] Implement tags
- [ ] Implement stash
- [ ] Implement `checkout`, just change HEAD to some ref/commit_hash
- [ ] [Implement CLI interface](https://hackmd.io/@ML10QiX-T6uAH01BMEGq5w/S1tLMuv_eg)
- [ ] Added support for detached state (HEAD file contains hash of commit directly)
- [ ] Added support for non-detached state (HEAD file contains ref)
- [ ] `get_file_hash_for_commit` for staging area, to detect whether a file was modified
  - [ ] Parse object file content to an in-memory struct for easier access.
  

COMMIT OBJECT CONTENT:
```zsh
➜  a4 git:(a4946e2) cat 946e2feb06e2e2e338b39aee7c2b890cbf47d0
tree 6b0c405f362d7c20942341f39fba18e217f507f5
author apoorvapendse
committer rajeevtapadia

Is this the real life
Is this just fantasy
```
---------------------------------------------------------------------------------------
TREE OBJECT CONTENT:
```zsh
➜  6b git:(a4946e2) cat 0c405f362d7c20942341f39fba18e217f507f5
16384 tree 1d7e200148f3b648f4af053c06777184d5328357 subdir
33188 blob 89d69a3b673d7d7d5ab7ebf2bbd88d994b1cc633 README.txt
```
