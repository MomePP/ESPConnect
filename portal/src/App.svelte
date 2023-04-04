<script>
	import {
		onMount
	} from 'svelte';
	import Connect from './components/Connect.svelte';
	import SelectScan from './components/SelectScan.svelte';
	import Status from './components/Status.svelte';

	let data = {
		loading: true,
		connectStatus: {
			sent: false,
			success: true
		},
		selection: {
			direct_connect: false,
			selected: false,
			ssid: '',
			hidden: false
		},
		access_points: [] 
	}

	function setConnectSuccess(){
		data.connectStatus.sent = true;
		data.connectStatus.success = true;
	}

	function setConnectError(){
		data.connectStatus.sent = true;
		data.connectStatus.success = false;
	}

	function clearSelection() {
		data.selection.selected = false;
		data.selection.direct_connect = false;
		data.selection.ssid = "";
	}

	function selectAccessPoint(event) {
		data.selection.ssid = event.detail.ssid
		data.selection.hidden = event.detail.hidden
		if(event.detail.open){
			data.selection.direct_connect = true
		}
		data.selection.selected = true;
	}

	async function refresh (){ 
		data.loading = true
		await updateAccessPoints()
	}

	async function updateAccessPoints() {
		// const res = await fetch(`http://localhost:9000/espconnect/scan`);
		const res = await fetch(`/espconnect/scan`);
		if (res.status === 200) {
			data.access_points = await res.json();
			data.access_points = data.access_points.filter((thing, index, self) => self.findIndex(t => t.name === thing.name) === index)
			data.loading = false;
		}else if(res.status === 202) {
			setTimeout(updateAccessPoints, 2000);
		}
		return res;
	}

	onMount(async () => {
		try {
			await updateAccessPoints();
		} catch (err) {
			console.log(err);
		}
	});
</script>

<div class="container main-container">
	<div class="row">
		<div class="column text-center">
			<!-- <svg xmlns="http://www.w3.org/2000/svg" width="64" height="64" viewBox="0 0 24 24" class="logo"><path d="M5 12L3 12 3 21 12 21 12 19 5 19zM12 5L19 5 19 12 21 12 21 3 12 3z"></path></svg> -->
			<!-- <img class="logo" src="/assets/gogoboard.svg" alt="GoGo Board" > -->
			<svg class="logo" xmlns="http://www.w3.org/2000/svg" x="0px" y="0px" width="313px" height="93px" viewBox="0 0 313 93" enable-background="new 0 0 313 93" xml:space="preserve">  
				<image id="image0" width="313" height="93" x="0" y="0" href="data:image/png;base64,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" />
			</svg>
		</div>
	</div>
	<div class="row mb-2">
		<div class="column">
			<div class="card">
				{#if data.loading}
					<div class="container">
						<div class="row h-100">
							<div class="column text-center d-flex h-100">
								<div class="loader">
									<svg xmlns="http://www.w3.org/2000/svg" width="36" height="36" viewBox="0 0 24 24" style="fill: currentColor"><path d="M12,22c5.421,0,10-4.579,10-10h-2c0,4.337-3.663,8-8,8s-8-3.663-8-8c0-4.336,3.663-8,8-8V2C6.579,2,2,6.58,2,12 C2,17.421,6.579,22,12,22z"></path></svg>
								</div>
							</div>
						</div>
					</div>
				{:else}
					{#if !data.connectStatus.sent}
						{#if !data.selection.selected}
							<SelectScan access_points={data.access_points} on:refresh={refresh} on:select={selectAccessPoint} />
						{:else}
							<Connect ssid={data.selection.ssid} hidden={data.selection.hidden} direct_connect={data.selection.direct_connect} on:back={clearSelection} on:success={setConnectSuccess} on:error={setConnectError} />
						{/if}
					{:else}
							<Status success={data.connectStatus.success} />
					{/if}
				{/if}
			</div>
		</div>
	</div>
	<div class="row">
		<div class="about column text-sm text-muted text-right">
			Made with <svg viewBox="0 0 1792 1792" preserveAspectRatio="xMidYMid meet" xmlns="http://www.w3.org/2000/svg" style="height: 0.8rem;"><path d="M896 1664q-26 0-44-18l-624-602q-10-8-27.5-26T145 952.5 77 855 23.5 734 0 596q0-220 127-344t351-124q62 0 126.5 21.5t120 58T820 276t76 68q36-36 76-68t95.5-68.5 120-58T1314 128q224 0 351 124t127 344q0 221-229 450l-623 600q-18 18-44 18z" fill="#e25555"></path></svg>
			<p class="about-details">
				<a href="https://github.com/ayushsharma82" target="_blank" rel="noreferrer">ayushsharma82</a><br>
				<a href="https://github.com/atfox98" target="_blank" rel="noreferrer">atfox98</a><br>
				<a href="https://github.com/momepp" target="_blank" rel="noreferrer">momepp</a>
			</p>
		</div>
	</div>
</div>

<style type="text/scss" global>
	$color-primary: #353a41;
	$color-secondary: #202327;
	$color-muted: #626770;
	$color-quinary: #869ab8;
	
	$loader-color: #fff;
	$loader-size: 8px;
	$loader-height: 4px;
	$loader-border-size: 4px;
	$loader-gap: 6px;
	$loader-animation-duration: 1s;

	@import "../node_modules/milligram/src/milligram.sass";
	@import "../node_modules/spinthatshit/src/variables.scss";
	@import "../node_modules/spinthatshit/src/animations.scss";
	@import "../node_modules/spinthatshit/src/loaders/_loader10.scss";

	.text-muted{
		color: $color-muted !important;
	}

	.text-center{
		text-align: center;
	}

	.text-right{
		text-align: right;
	}

	.w-100{
		width: 100%;
	}

	.h-100{
		height: 100%;
	}

	.d-flex{
		display: flex !important;
	}

	.flex-columns{
		flex-direction: column;
	}

	.flex-rows{
		flex-direction: row;
	}

	.mb-2{
		margin-bottom: 2rem;
	}

	.pr-1{
		padding-right: 1rem;
	}

	.w-auto{
		width: auto !important;
	}

	.text-sm{
		font-size: 12px;
	}

	.clickable-row{
		padding: 1rem 0rem;
		border-bottom: 0.1rem solid #f4f5f6;
		transition: background-color .5s cubic-bezier(0.215, 0.610, 0.355, 1), box-shadow .5s cubic-bezier(0.215, 0.610, 0.355, 1);
		border-radius: .5rem;
		cursor: pointer;
		word-break: break-word;

		&:hover{
			box-shadow: rgba(99, 99, 99, 0.1) 0px 2px 8px 2px;
		}
	}

	input[type=text], input[type=password]{
		padding: 2.5rem 2rem !important;
		box-shadow: rgba(204, 219, 232, 0.2) 0px 3px 6px 1px inset, rgba(255, 255, 255, 0.4) 0px 0px 6px 6px inset;
		border: none;
		font-size: 16px !important;
	}

	input:disabled{
		background-color: rgba(47, 63, 82, 0.05);
		border: none;
	}

	.logo{
		margin: 5rem 1rem;
		fill: currentColor;
	}

	.main-container{
		max-width: 42rem !important;
		margin: 3rem auto;
	}

	.card{
		display: flex;
		position: relative;
		padding: 1rem;
		border-radius: 1rem;
		width: 100%;
		box-shadow: rgba(149, 157, 165, 0.15) 0px 8px 24px;
		min-height: 290px;

		.container{
			padding: 1rem 2rem;
		}
	}

	.button{
		border-radius: .5rem;
		transition: background-color .5s linear;
		box-shadow: rgba(149, 157, 165, 0.15) 0px 8px 24px !important;
		&[disabled]{
			opacity: 0.9;
		}
	}

	.btn-light{
		background-color:rgb(113, 121, 129) !important;
		border-color: rgb(113, 121, 129) !important;
	}

	.loader{
		margin: auto;
		svg {
			animation-name: spin;
			animation-duration: 1500ms;
			animation-iteration-count: infinite;
			animation-timing-function: linear; 
		}
	}

	@keyframes spin {
    from {
        transform:rotate(0deg);
    }
    to {
        transform:rotate(360deg);
    }
	}

	.btn-loader{
		margin: auto;
		@include loader10;
	}

	.about-details{
		display: none;
	}

	.about:hover .about-details{
		display: block;
	}

</style>
