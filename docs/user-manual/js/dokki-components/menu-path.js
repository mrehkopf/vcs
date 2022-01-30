export const menuPath = {
    $tag: "menu-path",
    computed: {
        crumbs() {
            if (!this.$slots.default) {
                return [];
            }
            const keyString = this.$slots.default()[0].children;
            return keyString.split(" > ");
        }
    },
    template: `
    <span class="menu-path">
        <span v-for="(crumb, idx) in crumbs" :key="crumb">

            <span v-if="crumb == 'Context menu'">
                <a href="#output-window-context-menu">
                    <strong>{{crumb}}</strong>
                </a>
            </span>
            <strong v-else>{{crumb}}</strong>

            <i v-if="idx < (crumbs.length - 1)" class="fas fa-fw fa-caret-right"/>
            
        </span>
    </span>
    `,
};
