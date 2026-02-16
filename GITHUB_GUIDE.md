# Quick Start - Publishing to GitHub

This guide walks you through publishing Weather Lamp to GitHub.

## Prerequisites

- GitHub account (create at https://github.com/signup)
- Git installed on your computer
  - Windows: https://git-scm.com/download/win
  - Mac: `brew install git` or download from https://git-scm.com/download/mac
  - Linux: `sudo apt install git` or `sudo yum install git`

## Step-by-Step Guide

### 1. Create GitHub Repository

1. Go to https://github.com/new
2. Repository name: `weather-lamp` (or your preferred name)
3. Description: `ESP32 smart lamp displaying real-time weather via LED animations`
4. Visibility: **Public** (recommended) or Private
5. **Do NOT** initialize with README, .gitignore, or license (we already have these)
6. Click "Create repository"

### 2. Configure Git (First Time Only)

Open terminal/command prompt and configure your Git identity:

```bash
git config --global user.name "Your Name"
git config --global user.email "your.email@example.com"
```

### 3. Initialize Local Repository

Navigate to your project folder and initialize Git:

```bash
cd /path/to/weather-lamp
git init
```

### 4. Add All Files

Add all project files to Git:

```bash
git add .
```

This stages all files (`.gitignore` will prevent sensitive files from being added).

### 5. Create First Commit

Commit the files with a message:

```bash
git commit -m "Initial commit: Weather Lamp v1.0"
```

### 6. Connect to GitHub

Link your local repository to GitHub (replace `USERNAME` with your GitHub username):

```bash
git branch -M main
git remote add origin https://github.com/USERNAME/weather-lamp.git
```

### 7. Push to GitHub

Upload your code to GitHub:

```bash
git push -u origin main
```

You'll be prompted for GitHub credentials:
- Username: Your GitHub username
- Password: Use a **Personal Access Token** (not your password)
  - Generate at: https://github.com/settings/tokens
  - Scopes needed: `repo`

### 8. Verify

Visit your repository at: `https://github.com/USERNAME/weather-lamp`

You should see all files including README.md displayed!

---

## Making Updates

### After Changing Code

```bash
# Check what changed
git status

# Add changed files
git add filename.ino
# or add all changes
git add .

# Commit with descriptive message
git commit -m "Fixed WiFi reconnection bug"

# Push to GitHub
git push
```

### Creating Releases

When you have a stable version:

1. Go to your GitHub repository
2. Click "Releases" → "Draft a new release"
3. Tag version: `v1.0.0`
4. Release title: `Version 1.0.0 - Initial Release`
5. Description: Summarize changes (copy from CHANGELOG.md)
6. Attach compiled .bin files (optional)
7. Click "Publish release"

---

## Protecting Credentials

### Before Pushing, Verify:

```bash
# Check what will be committed
git diff

# Look for sensitive data
grep -r "YOUR_ACTUAL_PASSWORD" .
```

### If You Accidentally Commit Secrets:

**Option 1: Fix in next commit (if not pushed yet)**
```bash
git reset HEAD~1
# Edit files to remove secrets
git add .
git commit -m "Initial commit"
```

**Option 2: If already pushed (requires force push)**
```bash
# Edit files to remove secrets
git add .
git commit --amend
git push --force
```

⚠️ **Better**: Delete repository and start over if secrets were exposed!

---

## Branch Strategy (Optional)

### Create Development Branch

```bash
git checkout -b develop
# Make changes
git add .
git commit -m "Added new feature"
git push -u origin develop
```

### Merge to Main

```bash
git checkout main
git merge develop
git push
```

### Create Feature Branch

```bash
git checkout -b feature/web-config
# Work on feature
git add .
git commit -m "Added web configuration portal"
git push -u origin feature/web-config
```

Then create Pull Request on GitHub to merge into main.

---

## Useful Git Commands

### Check Status
```bash
git status
```

### View Commit History
```bash
git log
git log --oneline
```

### Undo Changes
```bash
# Discard changes to a file
git checkout -- filename.ino

# Undo last commit (keep changes)
git reset --soft HEAD~1

# Undo last commit (discard changes)
git reset --hard HEAD~1
```

### View Differences
```bash
git diff
git diff filename.ino
```

### Clone Repository
```bash
git clone https://github.com/USERNAME/weather-lamp.git
```

### Pull Latest Changes
```bash
git pull
```

---

## Collaborating

### Accepting Pull Requests

1. Review code changes on GitHub
2. Test locally if needed:
   ```bash
   git fetch origin
   git checkout pull-request-branch
   ```
3. Merge on GitHub if approved
4. Pull changes locally:
   ```bash
   git pull
   ```

### Forking Workflow

For contributors:
1. Fork repository on GitHub
2. Clone their fork
3. Make changes
4. Push to their fork
5. Create Pull Request to your repository

---

## GitHub Features to Enable

### Actions (CI/CD)
- Already configured in `.github/workflows/arduino-ci.yml`
- Automatically tests code on every push
- Enable in Settings → Actions

### Issues
- Bug tracking
- Feature requests
- Enable in Settings → Features

### Discussions
- Community Q&A
- Enable in Settings → Features

### Wiki
- Extended documentation
- Enable in Settings → Features

### Projects
- Kanban board for task management
- Create at Projects tab

---

## README Customization

Before publishing, update `README.md`:

1. Replace `yourusername` with your actual GitHub username:
   ```markdown
   git clone https://github.com/yourusername/weather-lamp.git
   ```

2. Add screenshots/photos:
   - Create `images/` folder
   - Add photos: `images/weather-lamp-demo.jpg`
   - Reference in README

3. Update contact information

4. Add project status badge:
   ```markdown
   ![Build Status](https://github.com/USERNAME/weather-lamp/workflows/Arduino%20CI/badge.svg)
   ```

---

## .gitignore Explained

The `.gitignore` file prevents sensitive/unnecessary files from being committed:

```gitignore
# Build artifacts
*.bin
*.o

# IDE files
.vscode/

# Sensitive config (if using separate config file)
config.h

# macOS junk
.DS_Store
```

---

## License

This project uses MIT License, meaning:
- ✅ Commercial use allowed
- ✅ Modification allowed
- ✅ Distribution allowed
- ✅ Private use allowed
- ⚠️ Liability and warranty: None

Others can use your code freely with attribution.

---

## Troubleshooting

### "Permission denied (publickey)"
- Set up SSH keys: https://docs.github.com/en/authentication/connecting-to-github-with-ssh
- Or use HTTPS with Personal Access Token

### "Failed to push some refs"
- Pull first: `git pull origin main`
- Resolve conflicts
- Then push: `git push`

### "Large files detected"
- GitHub has 100MB file limit
- Use Git LFS for large files
- Or remove large files from commit

### "Nothing to commit"
- Check if files are in `.gitignore`
- Use `git status` to see tracked files
- Use `git add -f filename` to force add

---

## Next Steps

After publishing to GitHub:

1. ✅ Add project to Arduino Library Manager (optional)
2. ✅ Share on Reddit (r/esp32, r/homeautomation, r/HomeKit)
3. ✅ Share on Hackster.io or Arduino Project Hub
4. ✅ Create demo video for YouTube
5. ✅ Write blog post about the project
6. ✅ Add to awesome-esp32 list

---

## Resources

- **Git Tutorial**: https://git-scm.com/docs/gittutorial
- **GitHub Guides**: https://guides.github.com/
- **Git Cheat Sheet**: https://education.github.com/git-cheat-sheet-education.pdf
- **Markdown Guide**: https://www.markdownguide.org/

---

**Ready to share your Weather Lamp with the world?** 🚀

Follow these steps and your project will be live on GitHub! If you run into issues, the GitHub community is very helpful.

**Good luck and happy coding!** 🌦️💡
