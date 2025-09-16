# 🚀 Git Subtree Cheat Sheet

### 🔹 Initial setup (add subtree)
```bash
git remote add <alias> https://github.com/<user>/<repo>.git
git subtree add --prefix=<folder> <alias> main --squash
```
Example:
```bash
git remote add uha_common https://github.com/martinkresta/UHA_COMMON.git
git subtree add --prefix=UhaCommon uha_common main --squash
```

---

### 🔹 Pull updates from library → app repo
```bash
git subtree pull --prefix=<folder> <alias> main --squash
```
Example:
```bash
git subtree pull --prefix=UhaCommon uha_common main --squash
```

---

### 🔹 Push updates from app repo → library
```bash
git subtree push --prefix=<folder> <alias> main
```
Example:
```bash
git subtree push --prefix=UhaCommon uha_common main
```

---

### 🔹 Safety / recovery
```bash
# Always make a branch before trying subtree operations
git checkout -b subtree-work

# If things break → just switch back
git checkout main
```

---

### 🔹 Tips
- `--squash` = keeps history in your app repo clean (one commit per pull).  
- If you want full library history → omit `--squash` (but history can get noisy).  
- You can reuse the same `<alias>` (`uha_common`) across all your app repos.  
