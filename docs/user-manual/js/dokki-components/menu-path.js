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

        <i class="fas fa-mouse-pointer fa-sm" style="margin-right: 0.3em"/>

        <span v-for="(crumb, idx) in crumbs" :key="crumb">

            <span v-if="crumb == 'Context'">
                <a href="#output-window-context-menu">
                    {{crumb}}
                </a>
            </span>
            <span v-else>
                {{crumb}}
            </span>

            <i v-if="idx < (crumbs.length - 1)" class="fas fa-fw fa-caret-right"/>
            
        </span>

    </span>
    `,
};
