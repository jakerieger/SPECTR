document.querySelectorAll('.osc-type-dropdown').forEach(dropdown => {
    const selected = dropdown.querySelector('.osc-type-dropdown-selected');
    const menu = dropdown.querySelector('.osc-type-dropdown-menu');
    const items = dropdown.querySelectorAll('.osc-type-dropdown-item');

    selected.addEventListener('click', () => {
        menu.classList.toggle('active');
    });

    items.forEach(item => {
        item.addEventListener('click', () => {
            selected.textContent = item.textContent;

            items.forEach(i => i.classList.remove('selected'));
            item.classList.add('selected');

            menu.classList.remove('active');

            // Get current value — use this to pass back to C++
            const currentValue = item.textContent.trim();
        });
    });

    document.addEventListener('click', (e) => {
        if (!dropdown.contains(e.target)) {
            menu.classList.remove('active');
        }
    });
});