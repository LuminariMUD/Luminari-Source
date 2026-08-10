/* Shared interactions and reference rendering for the DG Scripts guide. */
(function () {
  "use strict";

  var reference = window.DG_REFERENCE || {};

  function escapeHtml(value) {
    return String(value)
      .replace(/&/g, "&amp;")
      .replace(/</g, "&lt;")
      .replace(/>/g, "&gt;")
      .replace(/\"/g, "&quot;")
      .replace(/'/g, "&#039;");
  }

  function renderTriggerTables() {
    document.querySelectorAll("[data-dg-trigger-table]").forEach(function (table) {
      var type = table.getAttribute("data-dg-trigger-table");
      var rows = reference.triggerTypes && reference.triggerTypes[type];
      var body = table.querySelector("tbody");

      if (!rows || !body) {
        return;
      }

      body.innerHTML = rows.map(function (row) {
        var search = [row.name, row.narg, row.args, row.variables, row.behavior].join(" ").toLowerCase();
        return "<tr class=\"dg-reference-row\" data-search=\"" + escapeHtml(search) + "\">" +
          "<td><code>" + escapeHtml(row.name) + "</code></td>" +
          "<td>" + escapeHtml(row.narg) + "</td>" +
          "<td>" + escapeHtml(row.args) + "</td>" +
          "<td>" + escapeHtml(row.variables) + "</td>" +
          "<td>" + escapeHtml(row.behavior) + "</td>" +
          "</tr>";
      }).join("");
    });
  }

  function renderCoreCommands() {
    document.querySelectorAll("[data-dg-core-commands]").forEach(function (table) {
      var rows = reference.commands && reference.commands.core;
      var body = table.querySelector("tbody");

      if (!rows || !body) {
        return;
      }

      body.innerHTML = rows.map(function (row) {
        var search = [row.name, row.purpose, row.syntax].join(" ").toLowerCase();
        return "<tr class=\"dg-reference-row\" data-search=\"" + escapeHtml(search) + "\">" +
          "<td><code>" + escapeHtml(row.name) + "</code></td>" +
          "<td><code>" + escapeHtml(row.syntax) + "</code></td>" +
          "<td>" + escapeHtml(row.purpose) + "</td>" +
          "</tr>";
      }).join("");
    });
  }

  function renderClouds() {
    document.querySelectorAll("[data-dg-command-cloud]").forEach(function (cloud) {
      var group = cloud.getAttribute("data-dg-command-cloud");
      var values = reference.commands && reference.commands[group];

      if (!values) {
        return;
      }

      cloud.innerHTML = values.map(function (value) {
        return "<span class=\"dg-field\" data-search=\"" + escapeHtml(value.toLowerCase()) +
          "\"><code>" + escapeHtml(value) + "</code></span>";
      }).join("");
    });

    document.querySelectorAll("[data-dg-field-cloud]").forEach(function (cloud) {
      var group = cloud.getAttribute("data-dg-field-cloud");
      var values = reference.fields && reference.fields[group];

      if (!values) {
        return;
      }

      cloud.innerHTML = values.map(function (value) {
        return "<span class=\"dg-field\" data-search=\"" + escapeHtml(value.toLowerCase()) +
          "\"><code>" + escapeHtml(value) + "</code></span>";
      }).join("");
    });
  }

  function updateFilter(input) {
    var targetId = input.getAttribute("data-dg-filter-for");
    var target = targetId && document.getElementById(targetId);
    var wrapper = input.closest(".dg-filter");
    var counter = wrapper && wrapper.querySelector(".dg-filter__count");
    var query = input.value.trim().toLowerCase();
    var visible = 0;
    var items;

    if (!target) {
      return;
    }

    items = target.querySelectorAll("[data-search]");
    items.forEach(function (item) {
      var matches = !query || item.getAttribute("data-search").indexOf(query) !== -1;
      item.setAttribute("data-hidden", matches ? "false" : "true");
      if (matches) {
        visible += 1;
      }
    });

    if (counter) {
      counter.textContent = visible + " of " + items.length;
    }
  }

  function initializeFilters() {
    document.querySelectorAll("[data-dg-filter-for]").forEach(function (input) {
      input.addEventListener("input", function () {
        updateFilter(input);
      });
      updateFilter(input);
    });
  }

  function initializeNavigation() {
    var toggle = document.querySelector(".dg-nav-toggle");
    var nav = document.querySelector(".dg-nav");

    if (!toggle || !nav) {
      return;
    }

    function setOpen(open) {
      nav.setAttribute("data-open", open ? "true" : "false");
      toggle.setAttribute("aria-expanded", open ? "true" : "false");
    }

    toggle.addEventListener("click", function () {
      setOpen(nav.getAttribute("data-open") !== "true");
    });

    nav.addEventListener("click", function (event) {
      if (event.target.closest("a")) {
        setOpen(false);
      }
    });

    document.addEventListener("keydown", function (event) {
      if (event.key === "Escape") {
        setOpen(false);
        toggle.focus();
      }
    });

    window.addEventListener("resize", function () {
      if (window.innerWidth > 820) {
        setOpen(false);
      }
    });
  }

  function initializeCopyButtons() {
    document.querySelectorAll(".dg-copy").forEach(function (button) {
      button.addEventListener("click", function () {
        var code = button.closest(".dg-code");
        var pre = code && code.querySelector("pre");
        var original = button.textContent;

        if (!pre || !navigator.clipboard) {
          button.textContent = "Select code";
          return;
        }

        navigator.clipboard.writeText(pre.textContent.replace(/\n$/, "")).then(function () {
          button.textContent = "Copied";
          window.setTimeout(function () {
            button.textContent = original;
          }, 1400);
        }).catch(function () {
          button.textContent = "Select code";
        });
      });
    });
  }

  function initializeResponsiveSidebar() {
    var current = document.querySelector(".dg-sidebar > nav a[aria-current='location']");

    if (current && window.innerWidth <= 820) {
      current.scrollIntoView({ block: "nearest", inline: "center" });
    }
  }

  function initializeTableOfContents() {
    var links = Array.prototype.slice.call(document.querySelectorAll("[data-dg-toc] a[href^='#']"));
    var sections;
    var observer;

    if (!links.length || !("IntersectionObserver" in window)) {
      return;
    }

    sections = links.map(function (link) {
      return document.querySelector(link.getAttribute("href"));
    }).filter(Boolean);

    observer = new IntersectionObserver(function (entries) {
      var visible = entries.filter(function (entry) {
        return entry.isIntersecting;
      }).sort(function (a, b) {
        return a.boundingClientRect.top - b.boundingClientRect.top;
      });

      if (visible.length) {
        links.forEach(function (link) {
          link.toggleAttribute("aria-current", link.getAttribute("href") === "#" + visible[0].target.id);
          if (link.hasAttribute("aria-current")) {
            link.setAttribute("aria-current", "location");
          }
        });
      }
    }, { rootMargin: "-18% 0px -72% 0px" });

    sections.forEach(function (section) {
      observer.observe(section);
    });
  }

  document.addEventListener("DOMContentLoaded", function () {
    renderTriggerTables();
    renderCoreCommands();
    renderClouds();
    initializeFilters();
    initializeNavigation();
    initializeCopyButtons();
    initializeResponsiveSidebar();
    initializeTableOfContents();
  });
}());
