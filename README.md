Things to consider:

If you want to see more that just players and some bots, go beyond the presistet level, ints all of the levels, bots sit at different levels than people



Always use wayland, if using KDE (what I test on, kubuntu):
    Open System Settings in KDE.

    Go to Window Management -> Window Rules.

    Click + Add New...

    Description: Better Discord Overlay Fix

    Window class: Choose "Exact Match" and type Better Discord Overlay (or whatever you named your window in glfwCreateWindow).

    Click + Add Property and search for/add these:

        Accept focus: Set to Force and No.

        Focus stealing prevention: Set to Force and Extreme.

        Ignore global shortcuts: Set to Force and Yes.

        No titlebar and frame: Set to Force and Yes.

    Apply.