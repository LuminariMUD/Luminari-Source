# LuminariMUD Web Portal Documentation

This directory contains the public-facing web documentation portal for LuminariMUD, deployed via GitHub Pages.

## 📍 Live Site

Once deployed, the portal is accessible at:
- **Main URL**: https://luminarimud.github.io/Luminari-Source/
- **Portal**: https://luminarimud.github.io/Luminari-Source/web/

---

## 📁 Directory Structure

```
docs/web/
├── index.html              # Main landing page with sections for resources/guides
├── spells/
│   ├── by_class.html      # Interactive spell reference organized by class
│   └── reference.html     # Alphabetical spell reference
├── objects/
│   ├── index.html         # Object database search interface
│   └── README.md          # Object database documentation
├── guides/
│   └── oedit.html         # OEDIT Guide (converted from markdown)
├── data/
│   └── objects.json       # Object database data (generated from MySQL)
└── assets/
    ├── css/
    │   └── style.css          # Shared stylesheet for all pages
    ├── js/                    # JavaScript files (future)
    ├── img/                   # Images and graphics (future)
    └── pandoc-template.html   # Template for markdown→HTML conversion
```

---

## 🎨 Design System

### Color Palette
The web portal uses a consistent purple gradient theme:

- **Primary Gradient**: `#667eea` → `#764ba2`
- **Primary Color**: `#667eea` (Interactive elements, headings)
- **Secondary Color**: `#764ba2` (Accents)
- **Text Colors**: `#212529` (dark), `#495057` (medium), `#6c757d` (light)
- **Background**: `#f8f9fa` (light), `#e9ecef` (medium)

### Shared Stylesheet
All pages should link to the shared stylesheet:
```html
<link rel="stylesheet" href="../assets/css/style.css">
```

The stylesheet includes:
- CSS custom properties for theming
- Reusable components (buttons, cards, badges, alerts)
- Responsive grid system
- Typography scales
- Mobile-friendly breakpoints

---

## ➕ Adding New Content

### Adding a New HTML Page

1. **Create the HTML file** in the appropriate directory:
   ```bash
   # For a guide
   vim docs/web/guides/my-new-guide.html

   # For game content
   vim docs/web/content/my-content.html
   ```

2. **Use the shared stylesheet** in your HTML:
   ```html
   <!DOCTYPE html>
   <html lang="en">
   <head>
       <meta charset="UTF-8">
       <meta name="viewport" content="width=device-width, initial-scale=1.0">
       <title>My Page - LuminariMUD</title>
       <link rel="stylesheet" href="../assets/css/style.css">
   </head>
   <body>
       <div class="container">
           <div class="section">
               <!-- Your content here -->
           </div>
       </div>
   </body>
   </html>
   ```

3. **Add a link** to the landing page (`index.html`):
   ```html
   <a href="guides/my-new-guide.html" class="card">
       <h3>My New Guide</h3>
       <p class="description">Description of the guide...</p>
       <span class="badge">HTML</span>
   </a>
   ```

4. **Commit and push**:
   ```bash
   git add docs/web/
   git commit -m "Add new guide: My New Guide"
   git push origin master
   ```

---

## 📝 Converting Markdown to HTML

We use **Pandoc** to convert markdown documentation to styled HTML pages.

### Prerequisites
```bash
# Check if pandoc is installed
pandoc --version

# If not installed (Ubuntu/Debian)
sudo apt install pandoc
```

### Using the Pandoc Template

The template is located at `docs/web/assets/pandoc-template.html` and provides:
- Automatic styling with web portal theme
- Breadcrumb navigation
- Back-to-top button
- Responsive tables
- Syntax-highlighted code blocks
- Table of contents support

### Conversion Command

**Basic conversion:**
```bash
pandoc path/to/file.md \
  -o docs/web/guides/output.html \
  --template=docs/web/assets/pandoc-template.html \
  --metadata title="Page Title" \
  --metadata subtitle="Optional subtitle"
```

**With table of contents:**
```bash
pandoc path/to/file.md \
  -o docs/web/guides/output.html \
  --template=docs/web/assets/pandoc-template.html \
  --metadata title="Page Title" \
  --metadata subtitle="Optional subtitle" \
  --toc \
  --toc-depth=2
```

### Example: Converting OEDIT Guide

```bash
pandoc docs/world_game-data/OEDIT_GUIDE.md \
  -o docs/web/guides/oedit.html \
  --template=docs/web/assets/pandoc-template.html \
  --metadata title="OEDIT Guide for Builders" \
  --metadata subtitle="Complete reference for the object editor" \
  --toc \
  --toc-depth=2
```

### Batch Conversion Script

Create a script to convert multiple files:

```bash
#!/bin/bash
# convert-guides.sh

TEMPLATE="docs/web/assets/pandoc-template.html"

# Convert OEDIT guide
pandoc docs/world_game-data/OEDIT_GUIDE.md \
  -o docs/web/guides/oedit.html \
  --template=$TEMPLATE \
  --metadata title="OEDIT Guide" \
  --toc --toc-depth=2

# Convert Builder Manual
pandoc docs/world_game-data/builder_manual.md \
  -o docs/web/guides/builder-manual.html \
  --template=$TEMPLATE \
  --metadata title="Builder Manual" \
  --toc --toc-depth=2

# Add more conversions as needed...

echo "Conversion complete!"
```

Make it executable and run:
```bash
chmod +x convert-guides.sh
./convert-guides.sh
```

---

## 🔮 Updating Spell References

The spell HTML files are generated from the game's spell data using Python scripts.

### Generation Scripts

Located in `util/`:
- **`generate_spell_html.sh`** - Shell script wrapper
- **`generate_spell_html_detailed.py`** - Python script that queries MySQL and generates HTML

### Regenerating Spell Pages

```bash
cd util/

# Run the generation script
python3 generate_spell_html_detailed.py

# Move generated files to web directory
mv output/spells_by_class.html ../docs/web/spells/by_class.html
mv output/spells_reference.html ../docs/web/spells/reference.html

# Commit changes
git add ../docs/web/spells/
git commit -m "Update spell references"
git push
```

### Spell File Structure

Both spell files are **self-contained** with:
- Embedded CSS (inline `<style>` blocks)
- Embedded JavaScript (inline `<script>` blocks)
- No external dependencies

This makes them:
- Fast loading
- Fully functional offline
- Easy to distribute

---

## ⚔️ Updating Object Database

The object database is a static HTML/JavaScript interface that loads data from a JSON file.

### Architecture

Unlike the self-contained spell pages, the object database uses:
- **Static HTML** (`objects/index.html`) - Interface with embedded JavaScript
- **JSON Data File** (`data/objects.json`) - Database export
- **Shared CSS** - Uses web portal's `assets/css/style.css`

This separation allows updating the data without regenerating the entire page.

### Generation Script

Located in `util/`:
- **`export_objectdb_to_json.py`** - Python script that exports MySQL database to JSON

### Prerequisites

```bash
# Install pymysql module
sudo apt install python3-pymysql
```

### Regenerating Object Data

#### Using Existing Database Credentials

If you have `docs/TODO/objectdb/mysql.php` configured:

```bash
# Run the export script (auto-detects mysql.php)
python3 util/export_objectdb_to_json.py

# Verify the output
ls -lh docs/web/data/objects.json
```

#### Using Environment Variables

```bash
# Set database credentials
export DB_HOST=localhost
export DB_USER=your_username
export DB_PASSWORD=your_password
export DB_NAME=your_database

# Run the export
python3 util/export_objectdb_to_json.py
```

#### Custom Output Location

```bash
python3 util/export_objectdb_to_json.py /path/to/custom/output.json
```

### Committing Changes

```bash
# Add the generated JSON file
git add docs/web/data/objects.json

# Commit
git commit -m "Update object database data"
git push
```

### Object Database Features

- **Interactive Search**: Filter by name, type, material, zone, level
- **Advanced Filters**: Weapon groups, wear slots, bonus types
- **Client-Side Performance**: No server required, instant filtering
- **Mobile Responsive**: Works on all devices
- **Collapsible Sections**: Organized by object type

### Database Tables Used

The export script queries these tables:
- `object_database_items` - Main object data
- `object_database_wear_slots` - Equipment slots
- `object_database_bonuses` - Stat bonuses
- `object_database_obj_flags` - Object flags
- `object_database_perm_affects` - Permanent effects

### Excluded Content

The following zones are filtered out:
- Code Items (DO NOT EDIT)
- Builder Academy items
- Player port restrings
- PP (Player Port) exclusive items
- Unused/test zones

Items with the "Mold" flag are also excluded.

### Documentation

See `docs/web/objects/README.md` for detailed documentation on:
- Search features
- Database schema
- Weapon groups
- Troubleshooting
- Future enhancements

---

## 🎯 Best Practices

### File Naming Conventions

- **HTML files**: Use lowercase with hyphens: `my-guide.html`
- **Directories**: Use lowercase, singular: `guide/`, `spell/`, `content/`
- **Assets**: Descriptive names: `style.css`, `navigation.js`

### Where to Put Content

| Content Type | Directory | Example |
|--------------|-----------|---------|
| Builder guides | `guides/` | `oedit.html`, `builder-manual.html` |
| Spell references | `spells/` | `by_class.html`, `reference.html` |
| Object database | `objects/` | `index.html` |
| Game mechanics | `mechanics/` (create) | `combat.html`, `crafting.html` |
| Data files (JSON) | `data/` | `objects.json`, `spells.json` |
| CSS stylesheets | `assets/css/` | `style.css`, `custom.css` |
| JavaScript | `assets/js/` | `search.js`, `navigation.js` |
| Images | `assets/img/` | `logo.png`, `banner.jpg` |

### Content Guidelines

1. **Always link shared CSS**: Use `../assets/css/style.css` for consistency
2. **Mobile-first**: Test on mobile devices or use browser dev tools
3. **Semantic HTML**: Use proper heading hierarchy (h1 → h2 → h3)
4. **Accessibility**: Include alt text for images, proper ARIA labels
5. **Performance**: Optimize images, minimize inline styles
6. **SEO**: Include proper meta tags, descriptions, titles

### Testing Locally

Before pushing, test your pages:

```bash
# Simple HTTP server (Python 3)
cd docs/
python3 -m http.server 8000

# Visit in browser
open http://localhost:8000/web/
```

Or use VS Code's Live Server extension.

---

## 🚀 Deployment

### How It Works

GitHub Pages deployment is automated via GitHub Actions:

1. **Workflow File**: `.github/workflows/pages.yml`
2. **Trigger**: Automatic on push to `master` (changes to `docs/**`)
3. **Process**:
   - Checkout repository
   - Upload `docs/` directory as artifact
   - Deploy to GitHub Pages
4. **Build Time**: ~1-2 minutes
5. **Result**: Site live at `https://luminarimud.github.io/Luminari-Source/`

### Manual Deployment

Trigger manually from GitHub Actions:

1. Go to: https://github.com/LuminariMUD/Luminari-Source/actions
2. Click "Deploy GitHub Pages" workflow
3. Click "Run workflow" button
4. Select `master` branch
5. Click green "Run workflow" button

### Monitoring Deployment

Check deployment status:
- **Actions Tab**: https://github.com/LuminariMUD/Luminari-Source/actions
- **Look for**: 🟡 Yellow (running), ✅ Green (success), ❌ Red (failed)

### Important: .nojekyll File

The `docs/.nojekyll` file tells GitHub Pages **NOT to use Jekyll** for processing.

**Why this matters:**
- Without `.nojekyll`: Jekyll processes files, breaks custom HTML
- With `.nojekyll`: Files served as-is, custom styling works

**⚠️ DO NOT DELETE `docs/.nojekyll`**

---

## 🔗 URL Structure

Once deployed, pages are accessible at:

### Main Pages
```
https://luminarimud.github.io/Luminari-Source/           → Redirect page
https://luminarimud.github.io/Luminari-Source/web/       → Landing page
```

### Spell References
```
https://luminarimud.github.io/Luminari-Source/web/spells/by_class.html
https://luminarimud.github.io/Luminari-Source/web/spells/reference.html
```

### Object Database
```
https://luminarimud.github.io/Luminari-Source/web/objects/
https://luminarimud.github.io/Luminari-Source/web/data/objects.json
```

### Guides
```
https://luminarimud.github.io/Luminari-Source/web/guides/oedit.html
https://luminarimud.github.io/Luminari-Source/web/guides/builder-manual.html
```

### Assets
```
https://luminarimud.github.io/Luminari-Source/web/assets/css/style.css
https://luminarimud.github.io/Luminari-Source/web/assets/img/logo.png
```

---

## 🛠️ Maintenance

### Updating the Landing Page

Edit `docs/web/index.html` to:
- Add new sections
- Update links
- Modify card descriptions
- Add "Coming Soon" features

### Updating Shared Styles

Edit `docs/web/assets/css/style.css` to:
- Modify color scheme
- Add new components
- Update responsive breakpoints
- Add animations

**Note**: Changes to `style.css` affect **all pages** using it.

### Broken Links

Check for broken links periodically:
```bash
# Install linkchecker
pip install linkchecker

# Check links
linkchecker http://localhost:8000/web/
```

---

## 🔮 Future Enhancements

Ideas for expanding the web portal:

### Short Term
- [ ] Convert more markdown guides to HTML
- [ ] Add search functionality (JavaScript)
- [ ] Create class/race/feat databases
- [ ] Add mobile navigation menu
- [ ] Implement dark mode toggle

### Medium Term
- [ ] API documentation section
- [ ] Interactive character builder
- [ ] Equipment/loot database
- [ ] Server status widget
- [ ] News/changelog feed

### Long Term
- [ ] Custom domain (e.g., `docs.luminarimud.com`)
- [ ] Multi-language support
- [ ] PDF exports of guides
- [ ] Integration with in-game help system
- [ ] User authentication for builder tools

---

## 📚 Additional Resources

### External Documentation
- **GitHub Pages**: https://docs.github.com/en/pages
- **Pandoc Manual**: https://pandoc.org/MANUAL.html
- **HTML5 Guide**: https://developer.mozilla.org/en-US/docs/Web/HTML
- **CSS Reference**: https://developer.mozilla.org/en-US/docs/Web/CSS

### Internal Documentation
- **Main README**: `../../README.md`
- **Technical Docs**: `../TECHNICAL_DOCUMENTATION_MASTER_INDEX.md`
- **Build Guide**: `../guides/SETUP_AND_BUILD_GUIDE.md`

### Scripts & Utilities
- **Spell Generator**: `../../util/generate_spell_html_detailed.py`
- **Pandoc Template**: `assets/pandoc-template.html`
- **Deployment Workflow**: `../../.github/workflows/pages.yml`

---

## 🤝 Contributing

When adding content to the web portal:

1. **Follow the style guide** - Use consistent formatting
2. **Test locally first** - Don't push broken pages
3. **Update this README** - Document new sections/features
4. **Update the landing page** - Add links to new content
5. **Write good commit messages** - Explain what and why

### Commit Message Format
```
Add/Update: Brief description

- Detailed change 1
- Detailed change 2
- Why this change matters
```

---

## ❓ Troubleshooting

### Site not updating after push?

1. Check GitHub Actions: https://github.com/LuminariMUD/Luminari-Source/actions
2. Look for failed workflows (red X)
3. Click on failed workflow to see error logs
4. Common issues:
   - Missing `.nojekyll` file
   - Invalid HTML syntax
   - Broken links to assets

### Page showing 404?

1. Verify file exists in `docs/web/`
2. Check file path in URL (case-sensitive)
3. Ensure workflow completed successfully
4. Try hard refresh (Ctrl+Shift+R / Cmd+Shift+R)

### Styling broken?

1. Check CSS link path: `<link rel="stylesheet" href="../assets/css/style.css">`
2. Verify relative path is correct (../ goes up one directory)
3. Check browser console for 404 errors on CSS file

### Markdown not converting properly?

1. Verify pandoc is installed: `pandoc --version`
2. Check template path is correct
3. Ensure markdown file has valid syntax
4. Try conversion without `--toc` flag first

---

## 📞 Support

For questions or issues:
- **Discord**: https://discord.gg/Me3Tuu4
- **GitHub Issues**: https://github.com/LuminariMUD/Luminari-Source/issues
- **Email**: Check repository maintainers

---

**Last Updated**: November 2025
**Maintainers**: LuminariMUD Development Team
**Version**: 1.0
